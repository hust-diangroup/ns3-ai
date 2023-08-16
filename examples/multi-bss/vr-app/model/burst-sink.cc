/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "burst-sink.h"

#include "ns3/address-utils.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-socket.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BurstSink");

NS_OBJECT_ENSURE_REGISTERED(BurstSink);

TypeId
BurstSink::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::BurstSink")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<BurstSink>()
                            .AddAttribute("Local",
                                          "The Address on which to Bind the rx socket.",
                                          AddressValue(),
                                          MakeAddressAccessor(&BurstSink::m_local),
                                          MakeAddressChecker())
                            .AddAttribute("Protocol",
                                          "The type id of the protocol to use for the rx socket.",
                                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                                          MakeTypeIdAccessor(&BurstSink::m_tid),
                                          MakeTypeIdChecker())
                            .AddTraceSource("FragmentRx",
                                            "A fragment has been received",
                                            MakeTraceSourceAccessor(&BurstSink::m_rxFragmentTrace),
                                            "ns3::BurstSink::SeqTsSizeFragCallback")
                            .AddTraceSource("BurstRx",
                                            "A burst has been successfully received",
                                            MakeTraceSourceAccessor(&BurstSink::m_rxBurstTrace),
                                            "ns3::BurstSink::SeqTsSizeFragCallback");
    return tid;
}

BurstSink::BurstSink()
{
    NS_LOG_FUNCTION(this);
}

BurstSink::~BurstSink()
{
    NS_LOG_FUNCTION(this);
}

uint64_t
BurstSink::GetTotalRxBytes() const
{
    NS_LOG_FUNCTION(this);
    return m_totRxBytes;
}

uint64_t
BurstSink::GetTotalRxFragments() const
{
    NS_LOG_FUNCTION(this);
    return m_totRxFragments;
}

uint64_t
BurstSink::GetTotalRxBursts() const
{
    NS_LOG_FUNCTION(this);
    return m_totRxBursts;
}

Ptr<Socket>
BurstSink::GetListeningSocket(void) const
{
    NS_LOG_FUNCTION(this);
    return m_socket;
}

std::list<Ptr<Socket>>
BurstSink::GetAcceptedSockets(void) const
{
    NS_LOG_FUNCTION(this);
    return m_socketList;
}

void
BurstSink::DoDispose(void)
{
    NS_LOG_FUNCTION(this);
    m_socket = 0;
    m_socketList.clear();

    // chain up
    Application::DoDispose();
}

// Application Methods
void
BurstSink::StartApplication() // Called at time specified by Start
{
    NS_LOG_FUNCTION(this);
    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        if (m_socket->Bind(m_local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Listen();
        m_socket->ShutdownSend();
        if (addressUtils::IsMulticast(m_local))
        {
            Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
            if (udpSocket)
            {
                // equivalent to setsockopt (MCAST_JOIN_GROUP)
                udpSocket->MulticastJoinGroup(0, m_local);
            }
            else
            {
                NS_FATAL_ERROR("Error: joining multicast on a non-UDP socket");
            }
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&BurstSink::HandleRead, this));
    m_socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                MakeCallback(&BurstSink::HandleAccept, this));
    m_socket->SetCloseCallbacks(MakeCallback(&BurstSink::HandlePeerClose, this),
                                MakeCallback(&BurstSink::HandlePeerError, this));
}

void
BurstSink::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);
    while (!m_socketList.empty()) // these are accepted sockets, close them
    {
        Ptr<Socket> acceptedSocket = m_socketList.front();
        m_socketList.pop_front();
        acceptedSocket->Close();
    }
    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void
BurstSink::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> fragment;
    Address from;
    Address localAddress;
    while ((fragment = socket->RecvFrom(from)))
    {
        if (fragment->GetSize() == 0)
        { // EOF
            break;
        }
        m_totRxBytes += fragment->GetSize();

        std::stringstream addressStr;
        if (InetSocketAddress::IsMatchingType(from))
        {
            addressStr << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                       << InetSocketAddress::ConvertFrom(from).GetPort();
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            addressStr << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                       << Inet6SocketAddress::ConvertFrom(from).GetPort();
        }
        else
        {
            addressStr << "UNKNOWN ADDRESS TYPE";
        }

        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " burst sink received "
                               << fragment->GetSize() << " bytes from " << addressStr.str()
                               << " total Rx " << m_totRxBytes << " bytes");

        socket->GetSockName(localAddress);

        // handle received fragment
        auto itBuffer = m_burstHandlerMap.find(from); // rename m_burstBufferMap, itBuffer
        if (itBuffer == m_burstHandlerMap.end())
        {
            NS_LOG_LOGIC("New stream from " << from);
            itBuffer = m_burstHandlerMap.insert(std::make_pair(from, BurstHandler())).first;
        }
        FragmentReceived(itBuffer->second, fragment, from, localAddress);
    }
}

void
BurstSink::FragmentReceived(BurstHandler& burstHandler,
                            const Ptr<Packet>& f,
                            const Address& from,
                            const Address& localAddress)
{
    NS_LOG_FUNCTION(this << f);

    SeqTsSizeFragHeader header;
    f->PeekHeader(header);
    NS_ABORT_IF(header.GetSize() == 0);

    m_totRxFragments++;
    m_rxFragmentTrace(f,
                      from,
                      localAddress,
                      header); // TODO should fragment still include header in trace?

    NS_LOG_DEBUG("Get BurstHandler for from="
                 << from << " with m_currentBurstSeq=" << burstHandler.m_currentBurstSeq
                 << ", m_fragmentsMerged=" << burstHandler.m_fragmentsMerged
                 << ", m_unorderedFragments.size ()=" << burstHandler.m_unorderedFragments.size()
                 << ", m_burstBuffer.GetSize ()=" << burstHandler.m_burstBuffer->GetSize()
                 << ", for fragment with header: " << header);

    if (header.GetSeq() < burstHandler.m_currentBurstSeq)
    {
        NS_LOG_LOGIC("Ignoring fragment from previous burst. Fragment burst seq="
                     << header.GetSeq()
                     << ", current burst seq=" << burstHandler.m_currentBurstSeq);
        return;
    }

    if (header.GetSeq() > burstHandler.m_currentBurstSeq)
    {
        // fragment of new burst: discard previous burst if incomplete
        NS_LOG_LOGIC("Start mering new burst seq "
                     << header.GetSeq() << " (previous=" << burstHandler.m_currentBurstSeq << ")");

        burstHandler.m_currentBurstSeq = header.GetSeq();
        burstHandler.m_fragmentsMerged = 0;
        burstHandler.m_unorderedFragments.clear();
        burstHandler.m_burstBuffer = Create<Packet>(0);
    }

    if (header.GetSeq() == burstHandler.m_currentBurstSeq)
    {
        // fragment of current burst
        NS_ASSERT_MSG(header.GetFragSeq() >= burstHandler.m_fragmentsMerged,
                      header.GetFragSeq() << " >= " << burstHandler.m_fragmentsMerged);

        NS_LOG_DEBUG("fragment sequence=" << header.GetFragSeq() << ", fragments merged="
                                          << burstHandler.m_fragmentsMerged);
        if (header.GetFragSeq() == burstHandler.m_fragmentsMerged)
        {
            // following packet: merge it
            f->RemoveHeader(header);
            burstHandler.m_burstBuffer->AddAtEnd(f);
            burstHandler.m_fragmentsMerged++;
            NS_LOG_LOGIC("Fragments merged " << burstHandler.m_fragmentsMerged << "/"
                                             << header.GetFrags() << " for burst "
                                             << header.GetSeq());

            // if present, merge following unordered fragments
            auto nextFragmentIt = burstHandler.m_unorderedFragments.begin();
            while (
                nextFragmentIt !=
                    burstHandler.m_unorderedFragments.end() && // there are unordered packets
                nextFragmentIt->first ==
                    burstHandler.m_fragmentsMerged) // the following fragment was already received
            {
                Ptr<Packet> storedFragment = nextFragmentIt->second;
                storedFragment->RemoveHeader(header);
                burstHandler.m_burstBuffer->AddAtEnd(storedFragment);
                burstHandler.m_fragmentsMerged++;
                NS_LOG_LOGIC("Unordered fragments merged " << burstHandler.m_fragmentsMerged << "/"
                                                           << header.GetFrags() << " for burst "
                                                           << header.GetSeq());

                nextFragmentIt = burstHandler.m_unorderedFragments.erase(nextFragmentIt);
            }
        }
        else
        {
            // add to unordered fragments buffer
            NS_LOG_LOGIC("Add unordered fragment " << header.GetFragSeq() << " of burst "
                                                   << header.GetSeq() << " to buffer ");
            burstHandler.m_unorderedFragments.insert(
                std::pair<uint16_t, const Ptr<Packet>>(header.GetFragSeq(), f));
        }
    }

    // check if burst is complete
    if (burstHandler.m_fragmentsMerged == header.GetFrags())
    {
        // all fragments have been merged
        NS_ASSERT_MSG(burstHandler.m_burstBuffer->GetSize() == header.GetSize(),
                      burstHandler.m_burstBuffer->GetSize() << " == " << header.GetSize());

        NS_LOG_LOGIC("Burst received: " << header.GetFrags() << " fragments for a total of "
                                        << header.GetSize() << " B");
        m_totRxBursts++;
        m_rxBurstTrace(burstHandler.m_burstBuffer,
                       from,
                       localAddress,
                       header); // TODO header size does not include payload, why?
    }
}

void
BurstSink::HandlePeerClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

void
BurstSink::HandlePeerError(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

void
BurstSink::HandleAccept(Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION(this << s << from);
    s->SetRecvCallback(MakeCallback(&BurstSink::HandleRead, this));
    m_socketList.push_back(s);
}

} // Namespace ns3
