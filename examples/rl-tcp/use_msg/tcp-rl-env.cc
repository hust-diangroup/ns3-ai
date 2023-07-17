/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 */

#include <numeric>
#include <iostream>
#include "tcp-rl-env.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ns3::TcpTimeStepEnv");
NS_OBJECT_ENSURE_REGISTERED(TcpTimeStepEnv);

Ns3AiMsgInterface<TcpRlEnv, TcpRlAct> m_msgInterface =
    Ns3AiMsgInterface<TcpRlEnv, TcpRlAct>(false, false, true);

TcpTimeStepEnv::TcpTimeStepEnv()
{
//    std::cerr << "in TcpTimeStepEnv(), this = " << this << std::endl;
}

TcpTimeStepEnv::~TcpTimeStepEnv()
{
//    std::cerr << "in ~TcpTimeStepEnv(), this = " << this << std::endl;
}

TypeId
TcpTimeStepEnv::GetTypeId()
{
    static TypeId tid = TypeId ("ns3::TcpTimeStepGymEnv")
                            .SetParent<Object> ()
                            .SetGroupName ("Ns3Ai")
                            .AddConstructor<TcpTimeStepEnv> ()
        ;

    return tid;
}

void TcpTimeStepEnv::SetNodeId(uint32_t id)
{
  NS_LOG_FUNCTION(this);
  m_nodeId = id;
}

void TcpTimeStepEnv::SetSocketUuid(uint32_t id)
{
  NS_LOG_FUNCTION(this);
  m_socketUuid = id;
}

void TcpTimeStepEnv::TxPktTrace(Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>)
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

void TcpTimeStepEnv::RxPktTrace(Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>)
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

void TcpTimeStepEnv::ScheduleNextStateRead()
{

  Simulator::Schedule(m_timeStep, &TcpTimeStepEnv::ScheduleNextStateRead, this);

  m_msgInterface.cpp_send_begin();
  auto env = m_msgInterface.m_single_cpp2py_msg;
  env->socketUid = m_socketUuid;
  env->envType = 1;
  env->simTime_us = Simulator::Now().GetMicroSeconds();
  env->nodeId = m_nodeId;
  env->ssThresh = m_tcb->m_ssThresh;
  env->cWnd = m_tcb->m_cWnd;
  env->segmentSize = m_tcb->m_segmentSize;

  uint64_t bytesInFlightSum = std::accumulate(m_bytesInFlight.begin(), m_bytesInFlight.end(), 0);
  env->bytesInFlight = bytesInFlightSum;
  m_bytesInFlight.clear();

  uint64_t segmentsAckedSum = std::accumulate(m_segmentsAcked.begin(), m_segmentsAcked.end(), 0);
  env->segmentsAcked = segmentsAckedSum;
  m_segmentsAcked.clear();
  std::cerr << "At " << (uint64_t)(Simulator::Now().GetMilliSeconds()) << "ms:\n";
  std::cerr << "\tstate --"
            << " ssThresh=" << env->ssThresh
            << " cWnd=" << env->cWnd
            << " segmentSize=" << env->segmentSize
            << " segmentAcked=" << env->segmentsAcked
            << " bytesInFlightSum=" << bytesInFlightSum
            << std::endl;
  m_msgInterface.cpp_send_end();

  m_msgInterface.cpp_recv_begin();
  auto act = m_msgInterface.m_single_py2cpp_msg;
  m_new_cWnd = act->new_cWnd;
  m_new_ssThresh = act->new_ssThresh;
  m_msgInterface.cpp_recv_end();

  std::cerr << "\taction --"
            << " new_cWnd=" << m_new_cWnd
            << " new_ssThresh=" << m_new_ssThresh
            << std::endl;
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
    // Notify();
    ScheduleNextStateRead();
  }

  // action
  return m_new_ssThresh;
}

void TcpTimeStepEnv::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
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
    // Notify();
    ScheduleNextStateRead();
  }
  // action
  tcb->m_cWnd = m_new_cWnd;
}

void TcpTimeStepEnv::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  //   NS_LOG_FUNCTION (this);
  //   NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " PktsAcked, SegmentsAcked: " << segmentsAcked << " Rtt: " << rtt);
  m_tcb = tcb;
  m_rttSum += rtt;
  m_rttSampleNum++;
}

void TcpTimeStepEnv::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                        const TcpSocketState::TcpCongState_t newState)
{
  //   NS_LOG_FUNCTION (this);
  //   std::string stateName = GetTcpCongStateName(newState);
  //   NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CongestionStateSet: " << newState << " " << stateName);
  m_tcb = tcb;
}

void TcpTimeStepEnv::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
  //   NS_LOG_FUNCTION (this);
  //   std::string eventName = GetTcpCAEventName(event);
  //   NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CwndEvent: " << event << " " << eventName);
  m_tcb = tcb;
}

} // namespace ns3