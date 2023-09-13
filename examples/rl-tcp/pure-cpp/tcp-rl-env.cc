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

#include "tcp-rl-env.h"

#include <iostream>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("tcp-rl-env-purecpp");

NS_OBJECT_ENSURE_REGISTERED(TcpTimeStepEnv);

TcpTimeStepEnv::TcpTimeStepEnv()
    : m_agent()
{
}

TcpTimeStepEnv::~TcpTimeStepEnv()
{
}

TypeId
TcpTimeStepEnv::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpTimeStepEnv")
                            .SetParent<Object>()
                            .SetGroupName("Ns3Ai")
                            .AddConstructor<TcpTimeStepEnv>()
                            .AddAttribute("StepTime",
                                          "Step interval used in TCP env. Default: 100ms",
                                          TimeValue(MilliSeconds(100)),
                                          MakeTimeAccessor(&TcpTimeStepEnv::m_timeStep),
                                          MakeTimeChecker());

    return tid;
}

void
TcpTimeStepEnv::SetNodeId(uint32_t id)
{
    NS_LOG_FUNCTION(this);
    m_nodeId = id;
}

void
TcpTimeStepEnv::SetSocketUuid(uint32_t id)
{
    NS_LOG_FUNCTION(this);
    m_socketUuid = id;
}

void
TcpTimeStepEnv::TxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>)
{
    //   NS_LOG_FUNCTION (this);
    if (m_lastPktTxTime > MicroSeconds(0.0))
    {
        Time interTxTime = Simulator::Now() - m_lastPktTxTime;
        m_interTxTimeSum += interTxTime;
        m_interTxTimeNum++;
    }

    m_lastPktTxTime = Simulator::Now();
}

void
TcpTimeStepEnv::RxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>)
{
    //   NS_LOG_FUNCTION (this);
    if (m_lastPktRxTime > MicroSeconds(0.0))
    {
        Time interRxTime = Simulator::Now() - m_lastPktRxTime;
        m_interRxTimeSum += interRxTime;
        m_interRxTimeNum++;
    }

    m_lastPktRxTime = Simulator::Now();
}

void
TcpTimeStepEnv::ScheduleNotify()
{
    Simulator::Schedule(m_timeStep, &TcpTimeStepEnv::ScheduleNotify, this);

    uint64_t bytesInFlightSum = std::accumulate(m_bytesInFlight.begin(), m_bytesInFlight.end(), 0);
    m_bytesInFlight.clear();

    uint64_t segmentsAckedSum = std::accumulate(m_segmentsAcked.begin(), m_segmentsAcked.end(), 0);
    m_segmentsAcked.clear();

    std::cerr << "At " << (uint64_t)(Simulator::Now().GetMilliSeconds()) << "ms:\n";
    std::cerr << "\tstate --"
              << " ssThresh=" << m_tcb->m_ssThresh << " cWnd=" << m_tcb->m_cWnd
              << " segmentAcked=" << segmentsAckedSum << " segmentSize=" << m_tcb->m_segmentSize
              << " bytesInFlightSum=" << bytesInFlightSum << std::endl;

    auto actions = m_agent.GetAction(m_tcb->m_ssThresh,
                                     m_tcb->m_cWnd,
                                     segmentsAckedSum,
                                     m_tcb->m_segmentSize,
                                     bytesInFlightSum);

    m_new_cWnd = std::get<0>(actions);
    m_new_ssThresh = std::get<1>(actions);

    std::cerr << "\taction --"
              << " new_cWnd=" << m_new_cWnd << " new_ssThresh=" << m_new_ssThresh << std::endl;

    m_rttSampleNum = 0;
    m_rttSum = MicroSeconds(0.0);

    m_interTxTimeNum = 0;
    m_interTxTimeSum = MicroSeconds(0.0);

    m_interRxTimeNum = 0;
    m_interRxTimeSum = MicroSeconds(0.0);
}

uint32_t
TcpTimeStepEnv::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
                                 << " GetSsThresh, BytesInFlight: " << bytesInFlight);
    m_tcb = tcb;
    m_bytesInFlight.push_back(bytesInFlight);

    if (!m_started)
    {
        m_started = true;
        ScheduleNotify();
    }

    return m_new_ssThresh;
}

void
TcpTimeStepEnv::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
                                 << " IncreaseWindow, SegmentsAcked: " << segmentsAcked);
    m_tcb = tcb;
    m_segmentsAcked.push_back(segmentsAcked);
    m_bytesInFlight.push_back(tcb->m_bytesInFlight);

    if (!m_started)
    {
        m_started = true;
        ScheduleNotify();
    }

    tcb->m_cWnd = m_new_cWnd;
}

void
TcpTimeStepEnv::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    m_tcb = tcb;
    m_rttSum += rtt;
    m_rttSampleNum++;
}

void
TcpTimeStepEnv::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState)
{
    m_tcb = tcb;
}

void
TcpTimeStepEnv::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
    m_tcb = tcb;
}

NS_OBJECT_ENSURE_REGISTERED(TcpEventBasedEnv);

TcpEventBasedEnv::TcpEventBasedEnv()
    : m_agent()
{
}

TcpEventBasedEnv::~TcpEventBasedEnv()
{
}

TypeId
TcpEventBasedEnv::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpEventBasedEnv")
                            .SetParent<Object>()
                            .SetGroupName("Ns3Ai")
                            .AddConstructor<TcpEventBasedEnv>();

    return tid;
}

void
TcpEventBasedEnv::SetNodeId(uint32_t id)
{
    NS_LOG_FUNCTION(this);
    m_nodeId = id;
}

void
TcpEventBasedEnv::SetSocketUuid(uint32_t id)
{
    NS_LOG_FUNCTION(this);
    m_socketUuid = id;
}

void
TcpEventBasedEnv::TxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>)
{
    //   NS_LOG_FUNCTION (this);
    if (m_lastPktTxTime > MicroSeconds(0.0))
    {
        Time interTxTime = Simulator::Now() - m_lastPktTxTime;
        m_interTxTimeSum += interTxTime;
        m_interTxTimeNum++;
    }

    m_lastPktTxTime = Simulator::Now();
}

void
TcpEventBasedEnv::RxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>)
{
    //   NS_LOG_FUNCTION (this);
    if (m_lastPktRxTime > MicroSeconds(0.0))
    {
        Time interRxTime = Simulator::Now() - m_lastPktRxTime;
        m_interRxTimeSum += interRxTime;
        m_interRxTimeNum++;
    }

    m_lastPktRxTime = Simulator::Now();
}

void
TcpEventBasedEnv::Notify()
{
    uint64_t bytesInFlightSum = std::accumulate(m_bytesInFlight.begin(), m_bytesInFlight.end(), 0);
    m_bytesInFlight.clear();

    uint64_t segmentsAckedSum = std::accumulate(m_segmentsAcked.begin(), m_segmentsAcked.end(), 0);
    m_segmentsAcked.clear();

    std::cerr << "At " << (uint64_t)(Simulator::Now().GetMilliSeconds()) << "ms:\n";
    std::cerr << "\tstate --"
              << " ssThresh=" << m_tcb->m_ssThresh << " cWnd=" << m_tcb->m_cWnd
              << " segmentAcked=" << segmentsAckedSum << " segmentSize=" << m_tcb->m_segmentSize
              << " bytesInFlightSum=" << bytesInFlightSum << std::endl;

    auto actions = m_agent.GetAction(m_tcb->m_ssThresh,
                                     m_tcb->m_cWnd,
                                     segmentsAckedSum,
                                     m_tcb->m_segmentSize,
                                     bytesInFlightSum);

    m_new_cWnd = std::get<0>(actions);
    m_new_ssThresh = std::get<1>(actions);

    std::cerr << "\taction --"
              << " new_cWnd=" << m_new_cWnd << " new_ssThresh=" << m_new_ssThresh << std::endl;

    m_rttSampleNum = 0;
    m_rttSum = MicroSeconds(0.0);

    m_interTxTimeNum = 0;
    m_interTxTimeSum = MicroSeconds(0.0);

    m_interRxTimeNum = 0;
    m_interRxTimeSum = MicroSeconds(0.0);
}

uint32_t
TcpEventBasedEnv::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
                                 << " GetSsThresh, BytesInFlight: " << bytesInFlight);
    m_tcb = tcb;
    m_bytesInFlight.push_back(bytesInFlight);

    Notify();

    return m_new_ssThresh;
}

void
TcpEventBasedEnv::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
                                 << " IncreaseWindow, SegmentsAcked: " << segmentsAcked);
    m_tcb = tcb;
    m_segmentsAcked.push_back(segmentsAcked);
    m_bytesInFlight.push_back(tcb->m_bytesInFlight);

    Notify();

    tcb->m_cWnd = m_new_cWnd;
}

void
TcpEventBasedEnv::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    m_tcb = tcb;
    m_rttSum += rtt;
    m_rttSampleNum++;
}

void
TcpEventBasedEnv::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                     const TcpSocketState::TcpCongState_t newState)
{
    m_tcb = tcb;
}

void
TcpEventBasedEnv::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
    m_tcb = tcb;
}

} // namespace ns3
