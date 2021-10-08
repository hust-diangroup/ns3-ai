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

#ifndef TCP_RL_H
#define TCP_RL_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include "tcp-rl-env.h"

namespace ns3 {

class TcpSocketBase;
class Time;

// used to get pointer to Congestion Algorithm
class TcpSocketDerived : public TcpSocketBase
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId () const;

  TcpSocketDerived (void);
  virtual ~TcpSocketDerived (void);

  Ptr<TcpCongestionOps> GetCongestionControlAlgorithm ();
};


class TcpRlTimeBased : public TcpCongestionOps
{
public:
  static TypeId GetTypeId (void);

  TcpRlTimeBased ();
  TcpRlTimeBased (const TcpRlTimeBased &sock);
  ~TcpRlTimeBased ();

  virtual std::string GetName () const;

  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt);
  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);
  virtual void CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event);
  virtual Ptr<TcpCongestionOps> Fork ();
  virtual void ReduceCwnd (Ptr<TcpSocketState> tcb);
protected:
  static uint64_t GenerateUuid ();
  virtual void CreateEnv ();
  void ConnectSocketCallbacks ();

  bool m_cbConnect{false};

  Ptr<TcpSocketBase> m_tcpSocket{0};

  Ptr<TcpTimeStepEnv> env;
  
  Time m_timeStep{MilliSeconds (100)};
};

} // namespace ns3

#endif /* TCP_RL_H */