#include <numeric>
#include "tcp-rl-env.h"
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

void TcpRlEnv::TxPktTrace(Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>)
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

void TcpRlEnv::RxPktTrace(Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>)
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

// void
// TcpEventEnv::UpdateEnv (void)
// {
//   auto env = EnvSetter ();
//   env->socketUid = m_socketUuid;
//   env->cWnd = m_tcb->m_cWnd;
//   env->ssThresh = m_tcb->m_ssThresh;
//   env->envType = 0;
//   env->simTime_us = Simulator::Now ().GetMicroSeconds ();
//   env->nodeId = m_nodeId;
//   env->ssThresh = m_tcb->m_ssThresh;
//   env->cWnd = m_tcb->m_cWnd;
//   env->segmentSize = m_tcb->m_segmentSize;
//   env->segmentsAcked = m_segmentsAcked;
//   env->bytesInFlight = m_bytesInFlight;
//   env->rtt = m_rtt.GetMicroSeconds ();
//   env->minRtt = m_tcb->m_minRtt.GetMicroSeconds ();
//   env->calledFunc = m_calledFunc;
//   env->congState = m_tcb->m_congState;
//   env->event = m_event;
//   env->ecnState = m_tcb->m_ecnState;
// }

// uint32_t
// TcpEventEnv::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
// {
//   //   NS_LOG_FUNCTION (this);
//   //   // pkt was lost, so penalty
//   m_envReward = m_penalty;

//   // //   NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " GetSsThresh, BytesInFlight: " << bytesInFlight);
//   m_calledFunc = CalledFunc_t::GET_SS_THRESH;
//   // //   m_info = "GetSsThresh";
//   m_tcb = tcb;
//   m_bytesInFlight = bytesInFlight;
//   SetCompleted ();
//   auto act = ActionGetter ();
//   // //   Notify();
//   //   return m_new_ssThresh;
// }

// void
// TcpEventEnv::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
// {
//   NS_LOG_FUNCTION (this);
//   // pkt was acked, so reward
//   m_envReward = m_reward;

//   NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId
//                                  << " IncreaseWindow, SegmentsAcked: " << segmentsAcked);
//   m_calledFunc = CalledFunc_t::INCREASE_WINDOW;
//   m_info = "IncreaseWindow";
//   m_tcb = tcb;
//   m_segmentsAcked = segmentsAcked;
//   Notify ();
//   tcb->m_cWnd = m_new_cWnd;
// }

// void
// TcpEventEnv::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
// {
//   NS_LOG_FUNCTION (this);
//   NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId << " PktsAcked, SegmentsAcked: "
//                                  << segmentsAcked << " Rtt: " << rtt);
//   m_calledFunc = CalledFunc_t::PKTS_ACKED;
//   m_info = "PktsAcked";
//   m_tcb = tcb;
//   m_segmentsAcked = segmentsAcked;
//   m_rtt = rtt;
// }

// void
// TcpEventEnv::CongestionStateSet (Ptr<TcpSocketState> tcb,
//                                     const TcpSocketState::TcpCongState_t newState)
// {
//   NS_LOG_FUNCTION (this);
//   std::string stateName = GetTcpCongStateName (newState);
//   NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId << " CongestionStateSet: " << newState
//                                  << " " << stateName);

//   m_calledFunc = CalledFunc_t::CONGESTION_STATE_SET;
//   m_info = "CongestionStateSet";
//   m_tcb = tcb;
//   m_newState = newState;
// }

// void
// TcpEventEnv::CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
// {
//   NS_LOG_FUNCTION (this);
//   std::string eventName = GetTcpCAEventName (event);
//   NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId << " CwndEvent: " << event << " "
//                                  << eventName);

//   m_calledFunc = CalledFunc_t::CWND_EVENT;
//   m_info = "CwndEvent";
//   m_tcb = tcb;
//   m_event = event;
// }

TcpTimeStepEnv::TcpTimeStepEnv(uint16_t id) : TcpRlEnv(id)
{
}

void TcpTimeStepEnv::ScheduleNextStateRead()
{
  //   NS_LOG_FUNCTION (this);
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
  m_bytesInFlight.clear();

  uint64_t segmentsAckedSum = std::accumulate(m_segmentsAcked.begin(), m_segmentsAcked.end(), 0);
  env->segmentsAcked = segmentsAckedSum;
  m_segmentsAcked.clear();
  std::cerr << (uint64_t)(Simulator::Now().GetMilliSeconds()) << "  " << env->ssThresh << "  "
            << env->cWnd << "  " << env->segmentSize << "  " << bytesInFlightSum << std::endl;
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
  std::cerr << Simulator::Now().GetMilliSeconds() << "  " << m_new_cWnd << "  " << m_new_ssThresh
            << std::endl;
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
