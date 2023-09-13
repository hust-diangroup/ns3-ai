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

#ifndef NS3_TCP_RL_H
#define NS3_TCP_RL_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include <ns3/ai-module.h>

namespace ns3
{

class TcpSocketBase;
class Time;
class TcpEnvBase;

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

class TcpRlBase : public TcpCongestionOps
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    TcpRlBase();

    /**
     * \brief Copy constructor.
     * \param sock object to copy.
     */
    TcpRlBase(const TcpRlBase& sock);

    ~TcpRlBase() override;

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
    virtual void CreateGymEnv();
    void ConnectSocketCallbacks();

    // OpenGymEnv interface
    Ptr<TcpSocketBase> m_tcpSocket;
    Ptr<TcpEnvBase> m_TcpEnvBase;
};

class TcpRlTimeBased : public TcpRlBase
{
  public:
    static TypeId GetTypeId();

    TcpRlTimeBased();
    TcpRlTimeBased(const TcpRlTimeBased& sock);
    ~TcpRlTimeBased() override;

    std::string GetName() const override;

  private:
    void CreateGymEnv() override;
};

class TcpRlEventBased : public TcpRlBase
{
  public:
    static TypeId GetTypeId();

    TcpRlEventBased();
    TcpRlEventBased(const TcpRlEventBased& sock);
    ~TcpRlEventBased() override;

    std::string GetName() const override;

  private:
    void CreateGymEnv() override;
    // OpenGymEnv env
    float m_reward{1.0};
    float m_penalty{-100.0};
};

} // namespace ns3

#endif // NS3_TCP_RL_H
