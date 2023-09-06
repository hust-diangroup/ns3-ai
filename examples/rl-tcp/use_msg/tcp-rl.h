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

#ifndef TCP_RL_H_MSG
#define TCP_RL_H_MSG

#include "tcp-rl-env.h"

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"

namespace ns3
{

class TcpSocketBase;
class Time;

// used to get pointer to Congestion Algorithm
class TcpSocketDerived : public TcpSocketBase
{
  public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    TcpSocketDerived();
    ~TcpSocketDerived() override;

    Ptr<TcpCongestionOps> GetCongestionControlAlgorithm();
};

class TcpRlTimeBased : public TcpCongestionOps
{
  public:
    static TypeId GetTypeId();

    TcpRlTimeBased();
    TcpRlTimeBased(const TcpRlTimeBased& sock);
    ~TcpRlTimeBased() override;

    std::string GetName() const override;

    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;
    void CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event) override;
    Ptr<TcpCongestionOps> Fork() override;

  protected:
    static uint64_t GenerateUuid();
    virtual void CreateEnv();
    void ConnectSocketCallbacks();

    bool m_cbConnect{false};

    TcpSocketBase* m_tcpSocket{nullptr};

    Ptr<TcpTimeStepEnv> env;
};

class TcpRlEventBased : public TcpCongestionOps
{
  public:
    static TypeId GetTypeId();

    TcpRlEventBased();
    TcpRlEventBased(const TcpRlEventBased& sock);
    ~TcpRlEventBased() override;

    std::string GetName() const override;

    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;
    void CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event) override;
    Ptr<TcpCongestionOps> Fork() override;

  protected:
    static uint64_t GenerateUuid();
    virtual void CreateEnv();
    void ConnectSocketCallbacks();

    bool m_cbConnect{false};

    TcpSocketBase* m_tcpSocket{nullptr};

    Ptr<TcpEventBasedEnv> env;
};

} // namespace ns3

#endif /* TCP_RL_H_MSG */
