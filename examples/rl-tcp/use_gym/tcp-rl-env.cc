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
 * Modify: Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#include "tcp-rl-env.h"

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-socket-base.h"

#include <numeric>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("tcp-rl-env-gym");

NS_OBJECT_ENSURE_REGISTERED(TcpEnvBase);

TcpEnvBase::TcpEnvBase()
{
    NS_LOG_FUNCTION(this);
    SetOpenGymInterface(OpenGymInterface::Get());
}

TcpEnvBase::~TcpEnvBase()
{
    NS_LOG_FUNCTION(this);
}

TypeId
TcpEnvBase::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpEnvBase").SetParent<OpenGymEnv>().SetGroupName("Ns3Ai");

    return tid;
}

void
TcpEnvBase::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
TcpEnvBase::SetNodeId(uint32_t id)
{
    NS_LOG_FUNCTION(this);
    m_nodeId = id;
}

void
TcpEnvBase::SetSocketUuid(uint32_t id)
{
    NS_LOG_FUNCTION(this);
    m_socketUuid = id;
}

std::string
TcpEnvBase::GetTcpCongStateName(const TcpSocketState::TcpCongState_t state)
{
    std::string stateName = "UNKNOWN";
    switch (state)
    {
    case TcpSocketState::CA_OPEN:
        stateName = "CA_OPEN";
        break;
    case TcpSocketState::CA_DISORDER:
        stateName = "CA_DISORDER";
        break;
    case TcpSocketState::CA_CWR:
        stateName = "CA_CWR";
        break;
    case TcpSocketState::CA_RECOVERY:
        stateName = "CA_RECOVERY";
        break;
    case TcpSocketState::CA_LOSS:
        stateName = "CA_LOSS";
        break;
    case TcpSocketState::CA_LAST_STATE:
        stateName = "CA_LAST_STATE";
        break;
    default:
        stateName = "UNKNOWN";
        break;
    }
    return stateName;
}

std::string
TcpEnvBase::GetTcpCAEventName(const TcpSocketState::TcpCAEvent_t event)
{
    std::string eventName = "UNKNOWN";
    switch (event)
    {
    case TcpSocketState::CA_EVENT_TX_START:
        eventName = "CA_EVENT_TX_START";
        break;
    case TcpSocketState::CA_EVENT_CWND_RESTART:
        eventName = "CA_EVENT_CWND_RESTART";
        break;
    case TcpSocketState::CA_EVENT_COMPLETE_CWR:
        eventName = "CA_EVENT_COMPLETE_CWR";
        break;
    case TcpSocketState::CA_EVENT_LOSS:
        eventName = "CA_EVENT_LOSS";
        break;
    case TcpSocketState::CA_EVENT_ECN_NO_CE:
        eventName = "CA_EVENT_ECN_NO_CE";
        break;
    case TcpSocketState::CA_EVENT_ECN_IS_CE:
        eventName = "CA_EVENT_ECN_IS_CE";
        break;
    case TcpSocketState::CA_EVENT_DELAYED_ACK:
        eventName = "CA_EVENT_DELAYED_ACK";
        break;
    case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
        eventName = "CA_EVENT_NON_DELAYED_ACK";
        break;
    default:
        eventName = "UNKNOWN";
        break;
    }
    return eventName;
}

/*
Define action space
*/
Ptr<OpenGymSpace>
TcpEnvBase::GetActionSpace()
{
    // new_ssThresh
    // new_cWnd
    uint32_t parameterNum = 2;
    float low = 0.0;
    float high = 65535;
    std::vector<uint32_t> shape = {
        parameterNum,
    };
    std::string dtype = TypeNameGet<uint32_t>();

    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(low, high, shape, dtype);
    NS_LOG_INFO("MyGetActionSpace: " << box);
    return box;
}

/*
Define game over condition
*/
bool
TcpEnvBase::GetGameOver()
{
    return false;
}

/*
Define reward function
*/
float
TcpEnvBase::GetReward()
{
    NS_LOG_INFO("MyGetReward: " << m_envReward);
    return m_envReward;
}

/*
Define extra info. Optional
*/
std::string
TcpEnvBase::GetExtraInfo()
{
    NS_LOG_INFO("MyGetExtraInfo: " << m_info);
    return m_info;
}

/*
Execute received actions
*/
bool
TcpEnvBase::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
    Ptr<OpenGymBoxContainer<uint32_t>> box = DynamicCast<OpenGymBoxContainer<uint32_t>>(action);
    m_new_ssThresh = box->GetValue(0);
    m_new_cWnd = box->GetValue(1);

    NS_LOG_INFO("MyExecuteActions: " << action);
    return true;
}

NS_OBJECT_ENSURE_REGISTERED(TcpTimeStepEnv);

TcpTimeStepEnv::TcpTimeStepEnv()
    : TcpEnvBase()
{
    NS_LOG_FUNCTION(this);
    m_envReward = 0.0;
}

void
TcpTimeStepEnv::ScheduleNextStateRead()
{
    NS_LOG_FUNCTION(this);
    Simulator::Schedule(m_timeStep, &TcpTimeStepEnv::ScheduleNextStateRead, this);
    Notify();
}

TcpTimeStepEnv::~TcpTimeStepEnv()
{
    NS_LOG_FUNCTION(this);
}

TypeId
TcpTimeStepEnv::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpTimeStepEnv")
                            .SetParent<TcpEnvBase>()
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
TcpTimeStepEnv::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

/*
Define observation space
*/
Ptr<OpenGymSpace>
TcpTimeStepEnv::GetObservationSpace()
{
    // socket unique ID
    // tcp env type: event-based = 0 / time-based = 1
    // sim time in us
    // node ID
    // ssThresh
    // cWnd
    // segmentSize
    // bytesInFlightSum
    // bytesInFlightAvg
    // segmentsAckedSum
    // segmentsAckedAvg
    // avgRtt
    // minRtt
    // avgInterTx
    // avgInterRx
    // throughput
    uint32_t parameterNum = 16;
    float low = 0.0;
    float high = 1000000000.0;
    std::vector<uint32_t> shape = {
        parameterNum,
    };
    std::string dtype = TypeNameGet<uint64_t>();

    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(low, high, shape, dtype);
    NS_LOG_INFO("MyGetObservationSpace: " << box);
    return box;
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer>
TcpTimeStepEnv::GetObservation()
{
    uint32_t parameterNum = 16;
    std::vector<uint32_t> shape = {
        parameterNum,
    };

    Ptr<OpenGymBoxContainer<uint64_t>> box = CreateObject<OpenGymBoxContainer<uint64_t>>(shape);

    box->AddValue(m_socketUuid);
    box->AddValue(1);
    box->AddValue(Simulator::Now().GetMicroSeconds());
    box->AddValue(m_nodeId);
    box->AddValue(m_tcb->m_ssThresh);
    box->AddValue(m_tcb->m_cWnd);
    box->AddValue(m_tcb->m_segmentSize);

    // bytesInFlightSum
    uint64_t bytesInFlightSum = std::accumulate(m_bytesInFlight.begin(), m_bytesInFlight.end(), 0);
    box->AddValue(bytesInFlightSum);

    // bytesInFlightAvg
    uint64_t bytesInFlightAvg = 0;
    if (!m_bytesInFlight.empty())
    {
        bytesInFlightAvg = bytesInFlightSum / m_bytesInFlight.size();
    }
    box->AddValue(bytesInFlightAvg);

    // segmentsAckedSum
    uint64_t segmentsAckedSum = std::accumulate(m_segmentsAcked.begin(), m_segmentsAcked.end(), 0);
    box->AddValue(segmentsAckedSum);

    // segmentsAckedAvg
    uint64_t segmentsAckedAvg = 0;
    if (!m_segmentsAcked.empty())
    {
        segmentsAckedAvg = segmentsAckedSum / m_segmentsAcked.size();
    }
    box->AddValue(segmentsAckedAvg);

    // avgRtt
    Time avgRtt = Seconds(0.0);
    if (m_rttSampleNum)
    {
        avgRtt = m_rttSum / m_rttSampleNum;
    }
    box->AddValue(avgRtt.GetMicroSeconds());

    // m_minRtt
    box->AddValue(m_tcb->m_minRtt.GetMicroSeconds());

    // avgInterTx
    Time avgInterTx = Seconds(0.0);
    if (m_interTxTimeNum)
    {
        avgInterTx = m_interTxTimeSum / m_interTxTimeNum;
    }
    box->AddValue(avgInterTx.GetMicroSeconds());

    // avgInterRx
    Time avgInterRx = Seconds(0.0);
    if (m_interRxTimeNum)
    {
        avgInterRx = m_interRxTimeSum / m_interRxTimeNum;
    }
    box->AddValue(avgInterRx.GetMicroSeconds());

    // throughput  bytes/s
    float throughput = (segmentsAckedSum * m_tcb->m_segmentSize) / m_timeStep.GetSeconds();
    box->AddValue(throughput);

    // Print data
    NS_LOG_INFO("MyGetObservation: " << box);

    m_bytesInFlight.clear();
    m_segmentsAcked.clear();

    m_rttSampleNum = 0;
    m_rttSum = MicroSeconds(0.0);

    m_interTxTimeNum = 0;
    m_interTxTimeSum = MicroSeconds(0.0);

    m_interRxTimeNum = 0;
    m_interRxTimeSum = MicroSeconds(0.0);

    return box;
}

void
TcpTimeStepEnv::TxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>)
{
    NS_LOG_FUNCTION(this);
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
    NS_LOG_FUNCTION(this);
    if (m_lastPktRxTime > MicroSeconds(0.0))
    {
        Time interRxTime = Simulator::Now() - m_lastPktRxTime;
        m_interRxTimeSum += interRxTime;
        m_interRxTimeNum++;
    }

    m_lastPktRxTime = Simulator::Now();
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
        //        Notify();
        ScheduleNextStateRead();
    }

    // action
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
        //        Notify();
        ScheduleNextStateRead();
    }
    // action
    tcb->m_cWnd = m_new_cWnd;
}

void
TcpTimeStepEnv::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " PktsAcked, SegmentsAcked: "
                                 << segmentsAcked << " Rtt: " << rtt);
    m_tcb = tcb;
    m_rttSum += rtt;
    m_rttSampleNum++;
}

void
TcpTimeStepEnv::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_FUNCTION(this);
    std::string stateName = GetTcpCongStateName(newState);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CongestionStateSet: " << newState
                                 << " " << stateName);
    m_tcb = tcb;
}

void
TcpTimeStepEnv::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
    NS_LOG_FUNCTION(this);
    std::string eventName = GetTcpCAEventName(event);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CwndEvent: " << event << " "
                                 << eventName);
    m_tcb = tcb;
}

NS_OBJECT_ENSURE_REGISTERED(TcpEventBasedEnv);

TcpEventBasedEnv::TcpEventBasedEnv()
    : TcpEnvBase()
{
    NS_LOG_FUNCTION(this);
}

TcpEventBasedEnv::~TcpEventBasedEnv()
{
    NS_LOG_FUNCTION(this);
}

TypeId
TcpEventBasedEnv::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpEventBasedEnv")
                            .SetParent<TcpEnvBase>()
                            .SetGroupName("Ns3Ai")
                            .AddConstructor<TcpEventBasedEnv>();

    return tid;
}

void
TcpEventBasedEnv::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
TcpEventBasedEnv::SetReward(float value)
{
    NS_LOG_FUNCTION(this);
    m_reward = value;
}

void
TcpEventBasedEnv::SetPenalty(float value)
{
    NS_LOG_FUNCTION(this);
    m_penalty = value;
}

/*
Define observation space
*/
Ptr<OpenGymSpace>
TcpEventBasedEnv::GetObservationSpace()
{
    // socket unique ID
    // tcp env type: event-based = 0 / time-based = 1
    // sim time in us
    // node ID
    // ssThresh
    // cWnd
    // segmentSize
    // segmentsAcked
    // bytesInFlight
    // rtt in us
    // min rtt in us
    // called func
    // congetsion algorithm (CA) state
    // CA event
    // ECN state
    uint32_t parameterNum = 15;
    float low = 0.0;
    float high = 1000000000.0;
    std::vector<uint32_t> shape = {
        parameterNum,
    };
    std::string dtype = TypeNameGet<uint64_t>();

    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(low, high, shape, dtype);
    NS_LOG_INFO("MyGetObservationSpace: " << box);
    return box;
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer>
TcpEventBasedEnv::GetObservation()
{
    uint32_t parameterNum = 15;
    std::vector<uint32_t> shape = {
        parameterNum,
    };

    Ptr<OpenGymBoxContainer<uint64_t>> box = CreateObject<OpenGymBoxContainer<uint64_t>>(shape);

    box->AddValue(m_socketUuid);
    box->AddValue(0);
    box->AddValue(Simulator::Now().GetMicroSeconds());
    box->AddValue(m_nodeId);
    box->AddValue(m_tcb->m_ssThresh);
    box->AddValue(m_tcb->m_cWnd);
    box->AddValue(m_tcb->m_segmentSize);
    box->AddValue(m_segmentsAcked);
    box->AddValue(m_bytesInFlight);
    box->AddValue(m_rtt.GetMicroSeconds());
    box->AddValue(m_tcb->m_minRtt.GetMicroSeconds());
    box->AddValue(m_calledFunc);
    box->AddValue(m_tcb->m_congState);
    box->AddValue(m_event);
    box->AddValue(m_tcb->m_ecnState);

    // Print data
    NS_LOG_INFO("MyGetObservation: " << box);
    return box;
}

void
TcpEventBasedEnv::TxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>)
{
    NS_LOG_FUNCTION(this);
}

void
TcpEventBasedEnv::RxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>)
{
    NS_LOG_FUNCTION(this);
}

uint32_t
TcpEventBasedEnv::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this);
    // pkt was lost, so penalty
    m_envReward = m_penalty;

    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
                                 << " GetSsThresh, BytesInFlight: " << bytesInFlight);
    m_calledFunc = CalledFunc_t::GET_SS_THRESH;
    m_info = "GetSsThresh";
    m_tcb = tcb;
    m_bytesInFlight = bytesInFlight;
    Notify();
    return m_new_ssThresh;
}

void
TcpEventBasedEnv::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this);
    // pkt was acked, so reward
    m_envReward = m_reward;

    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId
                                 << " IncreaseWindow, SegmentsAcked: " << segmentsAcked);
    m_calledFunc = CalledFunc_t::INCREASE_WINDOW;
    m_info = "IncreaseWindow";
    m_tcb = tcb;
    m_segmentsAcked = segmentsAcked;
    Notify();
    tcb->m_cWnd = m_new_cWnd;
}

void
TcpEventBasedEnv::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " PktsAcked, SegmentsAcked: "
                                 << segmentsAcked << " Rtt: " << rtt);
    m_calledFunc = CalledFunc_t::PKTS_ACKED;
    m_info = "PktsAcked";
    m_tcb = tcb;
    m_segmentsAcked = segmentsAcked;
    m_rtt = rtt;
}

void
TcpEventBasedEnv::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                     const TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_FUNCTION(this);
    std::string stateName = GetTcpCongStateName(newState);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CongestionStateSet: " << newState
                                 << " " << stateName);

    m_calledFunc = CalledFunc_t::CONGESTION_STATE_SET;
    m_info = "CongestionStateSet";
    m_tcb = tcb;
}

void
TcpEventBasedEnv::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
    NS_LOG_FUNCTION(this);
    std::string eventName = GetTcpCAEventName(event);
    NS_LOG_INFO(Simulator::Now() << " Node: " << m_nodeId << " CwndEvent: " << event << " "
                                 << eventName);

    m_calledFunc = CalledFunc_t::CWND_EVENT;
    m_info = "CwndEvent";
    m_tcb = tcb;
    m_event = event;
}

} // namespace ns3
