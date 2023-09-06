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

#ifndef TCP_RL_ENV_H_MSG
#define TCP_RL_ENV_H_MSG

#include "ns3/ai-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-socket-base.h"

namespace ns3
{
struct TcpRlEnv
{
    uint32_t nodeId;
    uint32_t socketUid;
    uint8_t envType;
    int64_t simTime_us;
    uint32_t ssThresh;
    uint32_t cWnd;
    uint32_t segmentSize;
    uint32_t segmentsAcked;
    uint32_t bytesInFlight;
};

struct TcpRlAct
{
    uint32_t new_ssThresh;
    uint32_t new_cWnd;
};

class TcpTimeStepEnv : public Object
{
  public:
    TcpTimeStepEnv();
    ~TcpTimeStepEnv() override;
    static TypeId GetTypeId();

    void SetNodeId(uint32_t id);
    void SetSocketUuid(uint32_t id);
    void TxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>);
    void RxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>);

    // TCP congestion control interface
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
    // optional functions used to collect obs
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt);
    void CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState);
    void CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event);

  private:
    uint32_t m_nodeId;
    uint32_t m_socketUuid;

    Time m_lastPktTxTime{MicroSeconds(0.0)};
    Time m_lastPktRxTime{MicroSeconds(0.0)};
    uint64_t m_interTxTimeNum{0};
    Time m_interTxTimeSum{MicroSeconds(0.0)};
    uint64_t m_interRxTimeNum{0};
    Time m_interRxTimeSum{MicroSeconds(0.0)};

    uint32_t m_new_ssThresh;
    uint32_t m_new_cWnd;
    void ScheduleNotify();
    bool m_started{false};
    Time m_timeStep;

    // state
    Ptr<const TcpSocketState> m_tcb;
    std::vector<uint32_t> m_bytesInFlight;
    std::vector<uint32_t> m_segmentsAcked;

    uint64_t m_rttSampleNum{0};
    Time m_rttSum{MicroSeconds(0.0)};
};

class TcpEventBasedEnv : public Object
{
  public:
    TcpEventBasedEnv();
    ~TcpEventBasedEnv() override;
    static TypeId GetTypeId();

    void SetNodeId(uint32_t id);
    void SetSocketUuid(uint32_t id);
    void TxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>);
    void RxPktTrace(Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>);

    // TCP congestion control interface
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
    // optional functions used to collect obs
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt);
    void CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState);
    void CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event);

  private:
    uint32_t m_nodeId;
    uint32_t m_socketUuid;

    Time m_lastPktTxTime{MicroSeconds(0.0)};
    Time m_lastPktRxTime{MicroSeconds(0.0)};
    uint64_t m_interTxTimeNum{0};
    Time m_interTxTimeSum{MicroSeconds(0.0)};
    uint64_t m_interRxTimeNum{0};
    Time m_interRxTimeSum{MicroSeconds(0.0)};

    uint32_t m_new_ssThresh;
    uint32_t m_new_cWnd;
    void Notify();

    // state
    Ptr<const TcpSocketState> m_tcb;
    std::vector<uint32_t> m_bytesInFlight;
    std::vector<uint32_t> m_segmentsAcked;

    uint64_t m_rttSampleNum{0};
    Time m_rttSum{MicroSeconds(0.0)};
};

} // namespace ns3

#endif /* TCP_RL_ENV_H_MSG */
