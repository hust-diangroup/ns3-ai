/*
 * Copyright (c) 2018 Technische Universität Berlin
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
 * Author: Piotr Gawlowicz <gawlowicz@tkn.tu-berlin.de>
 * Modify: Pengyu Liu <eic_lpy@hust.edu.cn>
 *         Hao Yin <haoyin@uw.edu>
 *         Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#include "tcp-rl.h"

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-socket-base.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("tcp-rl-msg");

NS_OBJECT_ENSURE_REGISTERED(TcpSocketDerived);

TypeId
TcpSocketDerived::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpSocketDerived")
                            .SetParent<TcpSocketBase>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpSocketDerived>();
    return tid;
}

TypeId
TcpSocketDerived::GetInstanceTypeId() const
{
    return TcpSocketDerived::GetTypeId();
}

TcpSocketDerived::TcpSocketDerived()
{
}

Ptr<TcpCongestionOps>
TcpSocketDerived::GetCongestionControlAlgorithm()
{
    return m_congestionControl;
}

TcpSocketDerived::~TcpSocketDerived()
{
}

/////////////////////////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(TcpRlTimeBased);

TcpRlTimeBased::TcpRlTimeBased()
    : TcpCongestionOps()
{
    NS_LOG_FUNCTION(this);
    //  std::cerr << "in TcpRlTimeBased (void), this = " << this << std::endl;
}

TcpRlTimeBased::TcpRlTimeBased(const TcpRlTimeBased& sock)
    : TcpCongestionOps(sock)
{
    NS_LOG_FUNCTION(this);
    //  std::cerr << "in TcpRlTimeBased (const TcpRlTimeBased &sock), this = " << this << std::endl;
}

TcpRlTimeBased::~TcpRlTimeBased()
{
    //  std::cerr << "in ~TcpRlTimeBased (void), this = " << this << std::endl;
}

TypeId
TcpRlTimeBased::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpRlTimeBased")
                            .SetParent<TcpSocketBase>()
                            .AddConstructor<TcpRlTimeBased>()
                            .SetGroupName("Internet");
    return tid;
}

uint64_t
TcpRlTimeBased::GenerateUuid()
{
    static uint64_t uuid = 0;
    uuid++;
    return uuid;
}

void
TcpRlTimeBased::CreateEnv()
{
    //  std::cerr << "in CreateEnv (), this = " << this << std::endl;
    NS_LOG_FUNCTION(this);
    env = CreateObject<TcpTimeStepEnv>();
    //  std::cerr << "CreateEnv" << (env == nullptr) << std::endl;
    env->SetSocketUuid(TcpRlTimeBased::GenerateUuid());

    ConnectSocketCallbacks();
}

void
TcpRlTimeBased::ConnectSocketCallbacks()
{
    //  std::cerr << "in ConnectSocketCallbacks (), this = " << this << std::endl;
    NS_LOG_FUNCTION(this);

    bool foundSocket = false;
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<TcpL4Protocol> tcp = node->GetObject<TcpL4Protocol>();

        ObjectVectorValue socketVec;
        tcp->GetAttribute("SocketList", socketVec);
        NS_LOG_DEBUG("Node: " << node->GetId() << " TCP socket num: " << socketVec.GetN());

        uint32_t sockNum = socketVec.GetN();
        for (uint32_t j = 0; j < sockNum; j++)
        {
            Ptr<Object> sockObj = socketVec.Get(j);
            Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase>(sockObj);
            NS_LOG_DEBUG("Node: " << node->GetId() << " TCP Socket: " << tcpSocket);
            if (!tcpSocket)
            {
                continue;
            }

            Ptr<TcpSocketDerived> dtcpSocket = StaticCast<TcpSocketDerived>(tcpSocket);
            Ptr<TcpCongestionOps> ca = dtcpSocket->GetCongestionControlAlgorithm();
            NS_LOG_DEBUG("CA name: " << ca->GetName());
            Ptr<TcpRlTimeBased> rlCa = DynamicCast<TcpRlTimeBased>(ca);
            if (rlCa == this)
            {
                NS_LOG_DEBUG("Found TcpRl CA!");
                foundSocket = true;
                //              m_tcpSocket = tcpSocket;
                m_tcpSocket = PeekPointer(tcpSocket);
                break;
            }
        }

        if (foundSocket)
        {
            break;
        }
    }

    NS_ASSERT_MSG(m_tcpSocket, "TCP socket was not found.");

    if (m_tcpSocket)
    {
        NS_LOG_DEBUG("Found TCP Socket: " << m_tcpSocket);
        m_tcpSocket->TraceConnectWithoutContext("Tx",
                                                MakeCallback(&TcpTimeStepEnv::TxPktTrace, env));
        m_tcpSocket->TraceConnectWithoutContext("Rx",
                                                MakeCallback(&TcpTimeStepEnv::RxPktTrace, env));
        NS_LOG_DEBUG("Connect socket callbacks " << m_tcpSocket->GetNode()->GetId());
        env->SetNodeId(m_tcpSocket->GetNode()->GetId());
    }
}

std::string
TcpRlTimeBased::GetName() const
{
    return "TcpRlTimeBased";
}

uint32_t
TcpRlTimeBased::GetSsThresh(Ptr<const TcpSocketState> state, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << state << bytesInFlight);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }

    uint32_t newSsThresh = env->GetSsThresh(state, bytesInFlight);

    return newSsThresh;
}

void
TcpRlTimeBased::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->IncreaseWindow(tcb, segmentsAcked);
}

void
TcpRlTimeBased::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->PktsAcked(tcb, segmentsAcked, rtt);
}

void
TcpRlTimeBased::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_FUNCTION(this);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->CongestionStateSet(tcb, newState);
}

void
TcpRlTimeBased::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
    NS_LOG_FUNCTION(this);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->CwndEvent(tcb, event);
}

Ptr<TcpCongestionOps>
TcpRlTimeBased::Fork()
{
    //  std::cerr << "in TcpRlTimeBased::Fork (), this = " << this << std::endl;
    return CopyObject<TcpRlTimeBased>(this);
}

NS_OBJECT_ENSURE_REGISTERED(TcpRlEventBased);

TypeId
TcpRlEventBased::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpRlEventBased")
                            .SetParent<TcpSocketBase>()
                            .AddConstructor<TcpRlEventBased>()
                            .SetGroupName("Internet");
    return tid;
}

TcpRlEventBased::TcpRlEventBased()
    : TcpCongestionOps()
{
}

TcpRlEventBased::TcpRlEventBased(const TcpRlEventBased& sock)
    : TcpCongestionOps(sock)
{
}

TcpRlEventBased::~TcpRlEventBased()
{
}

std::string
TcpRlEventBased::GetName() const
{
    return "TcpRlEventBased";
}

uint32_t
TcpRlEventBased::GetSsThresh(Ptr<const TcpSocketState> state, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << state << bytesInFlight);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }

    uint32_t newSsThresh = env->GetSsThresh(state, bytesInFlight);

    return newSsThresh;
}

void
TcpRlEventBased::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->IncreaseWindow(tcb, segmentsAcked);
}

void
TcpRlEventBased::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->PktsAcked(tcb, segmentsAcked, rtt);
}

void
TcpRlEventBased::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                    const TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_FUNCTION(this);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->CongestionStateSet(tcb, newState);
}

void
TcpRlEventBased::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
    NS_LOG_FUNCTION(this);
    if (!m_cbConnect)
    {
        m_cbConnect = true;
        CreateEnv();
    }
    env->CwndEvent(tcb, event);
}

Ptr<TcpCongestionOps>
TcpRlEventBased::Fork()
{
    return CopyObject<TcpRlEventBased>(this);
}

uint64_t
TcpRlEventBased::GenerateUuid()
{
    static uint64_t uuid = 0;
    uuid++;
    return uuid;
}

void
TcpRlEventBased::CreateEnv()
{
    //  std::cerr << "in CreateEnv (), this = " << this << std::endl;
    NS_LOG_FUNCTION(this);
    env = CreateObject<TcpEventBasedEnv>();
    std::cerr << "CreateEnv" << (env == nullptr) << std::endl;
    env->SetSocketUuid(TcpRlEventBased::GenerateUuid());

    ConnectSocketCallbacks();
}

void
TcpRlEventBased::ConnectSocketCallbacks()
{
    //  std::cerr << "in ConnectSocketCallbacks (), this = " << this << std::endl;
    NS_LOG_FUNCTION(this);

    bool foundSocket = false;
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<TcpL4Protocol> tcp = node->GetObject<TcpL4Protocol>();

        ObjectVectorValue socketVec;
        tcp->GetAttribute("SocketList", socketVec);
        NS_LOG_DEBUG("Node: " << node->GetId() << " TCP socket num: " << socketVec.GetN());

        uint32_t sockNum = socketVec.GetN();
        for (uint32_t j = 0; j < sockNum; j++)
        {
            Ptr<Object> sockObj = socketVec.Get(j);
            Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase>(sockObj);
            NS_LOG_DEBUG("Node: " << node->GetId() << " TCP Socket: " << tcpSocket);
            if (!tcpSocket)
            {
                continue;
            }

            Ptr<TcpSocketDerived> dtcpSocket = StaticCast<TcpSocketDerived>(tcpSocket);
            Ptr<TcpCongestionOps> ca = dtcpSocket->GetCongestionControlAlgorithm();
            NS_LOG_DEBUG("CA name: " << ca->GetName());
            Ptr<TcpRlEventBased> rlCa = DynamicCast<TcpRlEventBased>(ca);
            if (rlCa == this)
            {
                NS_LOG_DEBUG("Found TcpRl CA!");
                foundSocket = true;
                //              m_tcpSocket = tcpSocket;
                m_tcpSocket = PeekPointer(tcpSocket);
                break;
            }
        }

        if (foundSocket)
        {
            break;
        }
    }

    NS_ASSERT_MSG(m_tcpSocket, "TCP socket was not found.");

    if (m_tcpSocket)
    {
        NS_LOG_DEBUG("Found TCP Socket: " << m_tcpSocket);
        m_tcpSocket->TraceConnectWithoutContext("Tx",
                                                MakeCallback(&TcpEventBasedEnv::TxPktTrace, env));
        m_tcpSocket->TraceConnectWithoutContext("Rx",
                                                MakeCallback(&TcpEventBasedEnv::RxPktTrace, env));
        NS_LOG_DEBUG("Connect socket callbacks " << m_tcpSocket->GetNode()->GetId());
        env->SetNodeId(m_tcpSocket->GetNode()->GetId());
    }
}

} // namespace ns3
