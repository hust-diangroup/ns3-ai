/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2021 SIGNET Lab, Department of Information Engineering,
// University of Padova
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "bursty-application.h"

#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/burst-generator.h"
#include "ns3/data-rate.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BurstyApplication");

NS_OBJECT_ENSURE_REGISTERED(BurstyApplication);

TypeId
BurstyApplication::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::BurstyApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<BurstyApplication>()
            .AddAttribute("FragmentSize",
                          "The size of packets sent in a burst including SeqTsSizeFragHeader",
                          UintegerValue(1200),
                          MakeUintegerAccessor(&BurstyApplication::m_fragSize),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("Remote",
                          "The address of the destination",
                          AddressValue(),
                          MakeAddressAccessor(&BurstyApplication::m_peer),
                          MakeAddressChecker())
            .AddAttribute("Local",
                          "The Address on which to bind the socket. If not set, it is generated "
                          "automatically.",
                          AddressValue(),
                          MakeAddressAccessor(&BurstyApplication::m_local),
                          MakeAddressChecker())
            .AddAttribute("BurstGenerator",
                          "The BurstGenerator used by this application",
                          PointerValue(0),
                          MakePointerAccessor(&BurstyApplication::m_burstGenerator),
                          MakePointerChecker<BurstGenerator>())
            .AddAttribute("Protocol",
                          "The type of protocol to use. This should be "
                          "a subclass of ns3::SocketFactory",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&BurstyApplication::m_socketTid),
                          MakeTypeIdChecker())
            .AddTraceSource("FragmentTx",
                            "A fragment of the burst is sent",
                            MakeTraceSourceAccessor(&BurstyApplication::m_txFragmentTrace),
                            "ns3::BurstSink::SeqTsSizeFragCallback")
            .AddTraceSource("BurstTx",
                            "A burst of packet is created and sent",
                            MakeTraceSourceAccessor(&BurstyApplication::m_txBurstTrace),
                            "ns3::BurstSink::SeqTsSizeFragCallback");
    return tid;
}

BurstyApplication::BurstyApplication()
    : m_socket(0),
      m_connected(false),
      m_totTxBursts(0),
      m_totTxFragments(0),
      m_totTxBytes(0)
{
    NS_LOG_FUNCTION(this);
}

BurstyApplication::~BurstyApplication()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Socket>
BurstyApplication::GetSocket(void) const
{
    NS_LOG_FUNCTION(this);
    return m_socket;
}

Ptr<BurstGenerator>
BurstyApplication::GetBurstGenerator(void) const
{
    return m_burstGenerator;
}

void
BurstyApplication::DoDispose(void)
{
    NS_LOG_FUNCTION(this);

    CancelEvents();
    m_socket = 0;
    m_burstGenerator = 0;

    // chain up
    Application::DoDispose();
}

void
BurstyApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_socketTid);
        int ret = -1;

        if (!m_local.IsInvalid())
        {
            NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                             InetSocketAddress::IsMatchingType(m_local)) ||
                                (InetSocketAddress::IsMatchingType(m_peer) &&
                                 Inet6SocketAddress::IsMatchingType(m_local)),
                            "Incompatible peer and local address IP version");
            ret = m_socket->Bind(m_local);
        }
        else
        {
            if (Inet6SocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind6();
            }
            else if (InetSocketAddress::IsMatchingType(m_peer) ||
                     PacketSocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind();
            }
        }

        if (ret == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        m_socket->Connect(m_peer);
        m_socket->SetAllowBroadcast(true);
        m_socket->ShutdownRecv();

        m_socket->SetConnectCallback(MakeCallback(&BurstyApplication::ConnectionSucceeded, this),
                                     MakeCallback(&BurstyApplication::ConnectionFailed, this));
    }

    // Ensure no pending event
    CancelEvents();
    SendBurst();
}

void
BurstyApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);

    CancelEvents();
    if (m_socket)
    {
        m_socket->Close();
    }
    else
    {
        NS_LOG_WARN("BurstyApplication found null socket to close in StopApplication");
    }
}

void
BurstyApplication::CancelEvents()
{
    NS_LOG_FUNCTION(this);

    // Cancel next burst event
    Simulator::Cancel(m_nextBurstEvent);
}

void
BurstyApplication::SendBurst()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_nextBurstEvent.IsExpired());

    // get burst info
    uint32_t burstSize = 0;
    Time period;
    // packets must be at least as big as the header
    while (burstSize < 24) // TODO: find a way to improve this
    {
        if (!m_burstGenerator->HasNextBurst())
        {
            NS_LOG_LOGIC("Burst generator has no next burst: stopping application");
            StopApplication();
            return;
        }

        std::tie(burstSize, period) = m_burstGenerator->GenerateBurst();
        NS_LOG_DEBUG("Generated burstSize=" << burstSize << ", period=" << period.As(Time::MS));
    }

    NS_ASSERT_MSG(period.IsPositive(),
                  "Period must be non-negative, instead found period=" << period.As(Time::S));

    // send packets for current burst
    SendFragmentedBurst(burstSize);

    // schedule next burst
    NS_LOG_DEBUG("Next burst scheduled in " << period.As(Time::S));
    m_nextBurstEvent = Simulator::Schedule(period, &BurstyApplication::SendBurst, this);
}

void
BurstyApplication::SendFragmentedBurst(uint32_t burstSize)
{
    NS_LOG_FUNCTION(this << burstSize);

    // prepare header
    SeqTsSizeFragHeader hdrTmp;

    NS_ABORT_MSG_IF(burstSize < hdrTmp.GetSerializedSize(),
                    burstSize << " < " << hdrTmp.GetSerializedSize());
    NS_ABORT_MSG_IF(m_fragSize < hdrTmp.GetSerializedSize(),
                    m_fragSize << " < " << hdrTmp.GetSerializedSize());

    // compute number of fragments and sizes
    uint32_t numFullFrags = burstSize / m_fragSize; // integer division
    uint32_t lastFragSize = burstSize % m_fragSize; // modulo

    uint32_t secondToLastFragSize = 0;
    if (numFullFrags > 0)
    {
        // if there is at least one full fragment, there exist a second-to-last of full size
        secondToLastFragSize = m_fragSize;
        numFullFrags--;
    }
    if (secondToLastFragSize > 0 &&                // there exist a second-to-last fragment
        lastFragSize > 0 &&                        // last smaller fragment is needed
        lastFragSize < hdrTmp.GetSerializedSize()) // the last fragment is below the minimum size
    {
        // reduce second-to-last fragment to make last fragment of minimum size
        secondToLastFragSize = m_fragSize + lastFragSize - hdrTmp.GetSerializedSize();
        lastFragSize =
            hdrTmp.GetSerializedSize(); // TODO packet with no payload: might be a problem
    }
    NS_ABORT_MSG_IF(0 < secondToLastFragSize && secondToLastFragSize < hdrTmp.GetSerializedSize(),
                    secondToLastFragSize << " < " << hdrTmp.GetSerializedSize());
    NS_ABORT_MSG_IF(0 < lastFragSize && lastFragSize < hdrTmp.GetSerializedSize(),
                    lastFragSize << " < " << hdrTmp.GetSerializedSize());

    // total number of fragments
    uint32_t totFrags = numFullFrags;
    if (secondToLastFragSize > 0)
    {
        totFrags++;
    }
    if (lastFragSize > 0)
    {
        totFrags++;
    }
    uint64_t burstPayload = burstSize - (hdrTmp.GetSerializedSize() * totFrags);
    uint64_t fullFragmentPayload = m_fragSize - hdrTmp.GetSerializedSize();
    NS_LOG_DEBUG("Current burst size: "
                 << burstSize << " B: " << totFrags << " fragments with total payload "
                 << burstPayload << " B. "
                 << "Sending fragments: " << numFullFrags << " x " << m_fragSize << "B, + "
                 << secondToLastFragSize << " B + " << lastFragSize << " B");

    Ptr<Packet> burst = Create<Packet>(burstPayload);
    // Trace before adding header, for consistency with BurstSink
    Address from, to;
    m_socket->GetSockName(from);
    m_socket->GetPeerName(to);

    // TODO improve
    hdrTmp.SetSeq(m_totTxBursts);
    hdrTmp.SetSize(burstPayload);
    hdrTmp.SetFrags(totFrags);
    hdrTmp.SetFragSeq(0);

    m_txBurstTrace(burst, from, to, hdrTmp);

    uint64_t fragmentStart = 0;
    uint16_t fragmentSeq = 0;
    for (uint32_t i = 0; i < numFullFrags; i++)
    {
        Ptr<Packet> fragment = Create<Packet>(fullFragmentPayload);
        fragmentStart += fullFragmentPayload;
        SendFragment(fragment, burstPayload, totFrags, fragmentSeq++);
    }

    if (secondToLastFragSize > 0)
    {
        uint64_t secondToLastFragPayload = secondToLastFragSize - hdrTmp.GetSerializedSize();
        Ptr<Packet> fragment = Create<Packet>(secondToLastFragPayload);
        fragmentStart += secondToLastFragPayload;
        SendFragment(fragment, burstPayload, totFrags, fragmentSeq++);
    }

    if (lastFragSize > 0)
    {
        uint64_t lastFragPayload = lastFragSize - hdrTmp.GetSerializedSize();
        Ptr<Packet> fragment = Create<Packet>(lastFragPayload);
        fragmentStart += lastFragPayload;
        SendFragment(fragment, burstPayload, totFrags, fragmentSeq++);
    }

    NS_ASSERT(fragmentStart == burst->GetSize());

    m_totTxBursts++;
}

void
BurstyApplication::SendFragment(Ptr<Packet> fragment,
                                uint64_t burstSize,
                                uint16_t totFrags,
                                uint16_t fragmentSeq)
{
    NS_LOG_FUNCTION(this << fragment << burstSize << totFrags << fragmentSeq);

    SeqTsSizeFragHeader header;
    header.SetSeq(m_totTxBursts);
    header.SetSize(burstSize);
    header.SetFrags(totFrags);
    header.SetFragSeq(fragmentSeq);
    fragment->AddHeader(header);

    uint32_t fragmentSize = fragment->GetSize();
    int actual = m_socket->Send(fragment);
    if (uint32_t(actual) == fragmentSize)
    {
        Address from, to;
        m_socket->GetSockName(from);
        m_socket->GetPeerName(to);

        m_txFragmentTrace(fragment,
                          from,
                          to,
                          header); // TODO should fragment already include header in trace?
        m_totTxFragments++;
        m_totTxBytes += fragmentSize;

        std::stringstream addressStr;
        if (InetSocketAddress::IsMatchingType(m_peer))
        {
            addressStr << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << " port "
                       << InetSocketAddress::ConvertFrom(m_peer).GetPort();
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peer))
        {
            addressStr << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << " port "
                       << Inet6SocketAddress::ConvertFrom(m_peer).GetPort();
        }
        else
        {
            addressStr << "UNKNOWN ADDRESS TYPE";
        }

        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S)
                               << " bursty application sent fragment of " << fragment->GetSize()
                               << " bytes to " << addressStr.str() << " with header=" << header);
    }
    else
    {
        NS_LOG_DEBUG("Unable to send fragment: fragment size=" << fragment->GetSize()
                                                               << ", socket sent=" << actual
                                                               << "; ignoring unexpected behavior");
    }
}

void
BurstyApplication::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = true;
}

void
BurstyApplication::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_FATAL_ERROR("Can't connect");
}

uint64_t
BurstyApplication::GetTotalTxBursts(void) const
{
    return m_totTxBursts;
}

uint64_t
BurstyApplication::GetTotalTxFragments(void) const
{
    return m_totTxFragments;
}

uint64_t
BurstyApplication::GetTotalTxBytes(void) const
{
    return m_totTxBytes;
}

} // Namespace ns3
