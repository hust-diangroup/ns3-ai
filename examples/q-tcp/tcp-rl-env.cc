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
 * Modified: Shayekh Bin Islam <shayekh.bin.islam@gmail.com>
 */

#include <numeric>
#include "tcp-rl-env.h"
// #include "ns3/flow-monitor-helper.h"
// #include "ns3/ipv4-flow-classifier.h"

// extern ns3::FlowMonitorHelper flowHelper;
// extern ns3::Ptr<ns3::FlowMonitor> monitor;

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ns3::TcpRlEnv");
TcpRlEnv::TcpRlEnv(uint16_t id) : Ns3AIRL<sTcpRlEnv, TcpRlAct>(id)
{
  SetCond(2, 0);
}
void TcpRlEnv::SetNodeId(uint32_t id)
{
  NS_LOG_FUNCTION(this);
  m_nodeId = id;
}

void TcpRlEnv::SetSocketUuid(uint32_t id)
{
  NS_LOG_FUNCTION(this);
  m_socketUuid = id;
}

void TcpRlEnv::TxPktTrace(Ptr<const Packet>, const TcpHeader &,
                          Ptr<const TcpSocketBase>)
{
  //   NS_LOG_FUNCTION (this);
  if (m_lastPktTxTime > MicroSeconds(0.0))
  {
    Time interTxTime = Simulator::Now() - m_lastPktTxTime;
    m_interTxTimeSum += interTxTime;
    m_interTxTimeNum++;
    m_interTxTimeAvg = m_alpha*m_interTxTimeAvg + (1 - m_alpha)*interTxTime;
  }

  m_lastPktTxTime = Simulator::Now();
}

void TcpRlEnv::RxPktTrace(Ptr<const Packet>, const TcpHeader &,
                          Ptr<const TcpSocketBase>)
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


TcpTimeStepEnv::TcpTimeStepEnv(uint16_t id) : TcpRlEnv(id)
{
}

void TcpTimeStepEnv::ScheduleNextStateRead()
{
  NS_LOG_FUNCTION (this);
  // std::cerr << m_timeStep.GetMilliSeconds() << std::endl;
  Simulator::Schedule(m_timeStep, &TcpTimeStepEnv::ScheduleNextStateRead, this);
  //   Notify();
  // while (GetVersion() % 2 != 0)
  //   ;
  auto env = EnvSetterCond();
  env->socketUid = m_socketUuid;
  env->envType = 1;
  env->simTime_us = Simulator::Now().GetMicroSeconds();
  env->nodeId = m_nodeId;
  env->ssThresh = m_tcb->m_ssThresh;
  env->cWnd = m_tcb->m_cWnd;
  env->segmentSize = m_tcb->m_segmentSize;
  uint64_t bytesInFlightSum = std::accumulate(m_bytesInFlight.begin(), m_bytesInFlight.end(), 0);
  env->bytesInFlight = bytesInFlightSum;
  
  // env->lastRtt = m_tcb->m_lastRtt.Get().GetInteger() - m_tcb->m_minRtt.GetInteger();
  env->lastRtt = m_tcb->m_lastRtt.Get().GetInteger();
  m_bytesInFlight.clear();
  uint64_t segmentsAckedSum = std::accumulate(m_segmentsAcked.begin(), m_segmentsAcked.end(), 0);
  env->segmentsAcked = segmentsAckedSum;
  m_segmentsAcked.clear();

  env->ackArrivalTime = m_ackArrivalTime;
  env->sendTime = m_interTxTimeAvg.GetMicroSeconds();

  env->delay = m_delay.GetMilliSeconds();
  env->throughput = m_throughput;

  env->rttRatio = 0.0;
  if (m_tcb->m_minRtt.GetInteger() != 0)
    env->rttRatio = m_tcb->m_lastRtt.Get().GetDouble() / m_tcb->m_minRtt.GetDouble();

  NS_LOG_INFO("m_rcvTimestampValue: " << m_tcb->m_rcvTimestampValue << " "
              << "m_rcvTimestampEchoReply: " << m_tcb->m_rcvTimestampEchoReply << " "
              << "lastRtt: " << m_tcb->m_lastRtt.Get().GetMilliSeconds() << "ms" << " "
              << "minrtt: " << m_tcb->m_minRtt.GetMilliSeconds() << "ms" << " "
              << "\n"
  );

  NS_LOG_INFO("env->ackArrivalTime: "  << env->ackArrivalTime << " "
              << "env->sendTime: "  << env->sendTime << " "
              << "env->rttRatio: " << env->rttRatio << " "
              << "\n");

  // NS3_LOG
  std::cerr << (uint64_t)(Simulator::Now().GetMilliSeconds()) << " ->  ssThresh: " << env->ssThresh << "  cWnd: "
            << env->cWnd << "  segmentSize: " << env->segmentSize << "  bytesInFlightSum: " << bytesInFlightSum << std::endl;


  
  // monitor->CheckForLostPackets ();
  // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier ());
  // FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

  // for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  // {
  //   Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
  //   ns3::int64x64_t throughput = (i->second.rxBytes * 8.0 /
  //                                   (i->second.timeLastRxPacket
  //                                    - i->second.timeFirstTxPacket).GetSeconds()) / 1000 / 1000;
  //   // this->
  //   NS_LOG_INFO(i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")"
  //               << "\n"
  //               << "Throughput: " << throughput
  //               << "\n"
  //   );
  // }
  // exit(0);


  SetCompleted();



  // std::cerr<<"GetVersion () "<<(int)GetVersion ()<<std::endl;
  // while (GetVersion() % 2 != 0)
  //   ;
  auto act = ActionGetterCond();
  m_new_cWnd = act->new_cWnd;
  m_new_ssThresh = act->new_ssThresh;
  GetCompleted();
  m_rttSampleNum = 0;
  m_rttSum = MicroSeconds(0.0);
  m_interTxTimeNum = 0;
  m_interTxTimeSum = MicroSeconds(0.0);
  m_interTxTimeAvg = MicroSeconds(0.0);
  m_interRxTimeNum = 0;
  m_interRxTimeSum = MicroSeconds(0.0);
  //   Time avgRtt = Seconds (0.0);
  //   if (m_rttSampleNum)
  //     avgRtt = m_rttSum / m_rttSampleNum;
  //   env->rtt = avgRtt.GetMicroSeconds ();
  //   env->minRtt = m_tcb->m_minRtt.GetMicroSeconds ();
  //   env->calledFunc = m_calledFunc;
  //   env->congState = m_tcb->m_congState;
  //   env->event = m_event;
  //   env->ecnState = m_tcb->m_ecnState;
  // DO_LOG_NS3
  // std::cerr << Simulator::Now().GetMilliSeconds() << " ->  m_new_cWnd: " << m_new_cWnd << "  m_new_ssThresh: " << m_new_ssThresh
  //           << std::endl;
}

uint32_t
TcpTimeStepEnv::GetSsThresh(Ptr<const TcpSocketState> tcb,
                            uint32_t bytesInFlight)
{
  m_lastCon = Simulator::Now ();

  NS_LOG_FUNCTION(this);
  NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
              << " GetSsThresh, BytesInFlight: " << bytesInFlight);
  m_tcb = tcb;
  m_bytesInFlight.push_back(bytesInFlight);

  m_lastThroughput = m_throughput;
  m_throughput = 0;
  m_dataSent = 0;

  if (!m_started)
  {
    m_started = true;
    // Notify();
    ScheduleNextStateRead();
  }

  // action
  return m_new_ssThresh;
}

void TcpTimeStepEnv::IncreaseWindow(Ptr<TcpSocketState> tcb,
                                    uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
              << " IncreaseWindow, SegmentsAcked: " << segmentsAcked);
  m_tcb = tcb;
  m_segmentsAcked.push_back(segmentsAcked);
  m_bytesInFlight.push_back(tcb->m_bytesInFlight);

  // Tcp-illinois
  if (tcb->m_lastAckedSeq >= m_endSeq)
  {
    // RecalcParam (tcb->m_cWnd);
    m_delay = m_rttSum / m_rttSampleNum - m_baseRtt;
    Reset (tcb->m_nextTxSequence);
  }

  if (!m_started)
  {
    m_started = true;
    // Notify();
    ScheduleNextStateRead();
  }

  // action
  tcb->m_cWnd = m_new_cWnd;
}

void
TcpTimeStepEnv::Reset (const SequenceNumber32 &nextTxSequence)
{
  NS_LOG_FUNCTION (this << nextTxSequence);
  m_endSeq = nextTxSequence;
  m_rttSampleNum = 0;
  m_rttSum = Time (0);
}

void TcpTimeStepEnv::PktsAcked(Ptr<TcpSocketState> tcb,
                               uint32_t segmentsAcked, const Time &rtt)
{
  
  //   NS_LOG_FUNCTION (this);
  Time now = Simulator::Now();
  m_tcb = tcb;
  m_rttSum += rtt;
  m_rttSampleNum++;
  
  // Similar to tcp-westwood
  m_ackArrivalTime = m_alpha * m_ackArrivalTime + (1 - m_alpha) * (now - m_lastAckTime).GetMilliSeconds();
  m_lastAckTime = now;


  // Tcp htcp: 
  m_baseRtt = std::min (m_baseRtt, rtt);

  // m_sendTime = m_alpha * m_sendTime + (1 - m_alpha) * (tcb->m_rcvTimestampEchoReply - m_lastSendTime);
  // m_lastSendTime = tcb->m_rcvTimestampEchoReply;

  // NS_LOG_INFO(
  //   "m_sendTime: " << m_sendTime << " "
  //   << "m_lastSendTime: " << m_lastSendTime << " "
  //   << "\n"
  // );
  if (tcb->m_congState == TcpSocketState::CA_OPEN)
  {
    m_dataSent += segmentsAcked * tcb->m_segmentSize;
  }

  m_throughput = static_cast<uint32_t> (m_dataSent / (now.GetSeconds () - m_lastCon.GetSeconds ()));

  NS_LOG_INFO(now << " Node: " << m_nodeId <<
              " PktsAcked, SegmentsAcked: " << segmentsAcked 
              << " Rtt: " << rtt << " "
              << " m_ackArrivalTime: " << m_ackArrivalTime << " ms" << " "
              << " m_throughput " << m_throughput << " "
              << " m_delay (ms): " << m_delay.GetMilliSeconds()
              << "\n"
  );
  
  // NS_LOG_INFO("m_rcvTimestampValue: " << tcb->m_rcvTimestampValue << " "
  //             << "m_rcvTimestampEchoReply: " << tcb->m_rcvTimestampEchoReply);
}

void TcpTimeStepEnv::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                        const TcpSocketState::TcpCongState_t newState)
{
  //   NS_LOG_FUNCTION (this);
  //   std::string stateName = GetTcpCongStateName(newState);
  //   NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CongestionStateSet: " << newState << " " << stateName);
  m_tcb = tcb;
}

void TcpTimeStepEnv::CwndEvent(Ptr<TcpSocketState> tcb,
                               const TcpSocketState::TcpCAEvent_t event)
{
  //   NS_LOG_FUNCTION (this);
  //   std::string eventName = GetTcpCAEventName(event);
  //   NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CwndEvent: " << event << " " << eventName);
  m_tcb = tcb;
}

} // namespace ns3
