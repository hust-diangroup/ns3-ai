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

#include "tcp-rl.h"
#include "ns3/tcp-header.h"
#include "ns3/object.h"
#include "ns3/node-list.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpSocketDerived);

TypeId
TcpSocketDerived::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketDerived")
                          .SetParent<TcpSocketBase> ()
                          .SetGroupName ("Internet")
                          .AddConstructor<TcpSocketDerived> ();
  return tid;
}

TypeId
TcpSocketDerived::GetInstanceTypeId () const
{
  return TcpSocketDerived::GetTypeId ();
}

TcpSocketDerived::TcpSocketDerived (void)
{
}

Ptr<TcpCongestionOps>
TcpSocketDerived::GetCongestionControlAlgorithm ()
{
  return m_congestionControl;
}

TcpSocketDerived::~TcpSocketDerived (void)
{
}
/////////////////////////////////////////////////////////////////////////
NS_LOG_COMPONENT_DEFINE ("ns3::TcpRlTimeBased");
NS_OBJECT_ENSURE_REGISTERED (TcpRlTimeBased);

TcpRlTimeBased::TcpRlTimeBased (void) : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
  m_tcpSocket = 0;
}

TcpRlTimeBased::TcpRlTimeBased (const TcpRlTimeBased &sock) : TcpCongestionOps (sock)
{
  NS_LOG_FUNCTION (this);
  m_tcpSocket = 0;
}

TcpRlTimeBased::~TcpRlTimeBased (void)
{
  m_tcpSocket = 0;
}

TypeId
TcpRlTimeBased::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::TcpRlTimeBased")
          .SetParent<TcpCongestionOps> ()
          .SetGroupName ("Internet")
          .AddConstructor<TcpRlTimeBased> ()
          .AddAttribute ("StepTime", "Step interval used in TCP env. Default: 100ms",
                         TimeValue (MilliSeconds (100)),
                         MakeTimeAccessor (&TcpRlTimeBased::m_timeStep), MakeTimeChecker ());
  return tid;
}
void
TcpRlTimeBased::ReduceCwnd (Ptr<TcpSocketState> tcb) 
{
   NS_LOG_FUNCTION (this << tcb);
   tcb->m_cWnd = std::max (tcb->m_cWnd.Get () / 2, tcb->m_segmentSize); 
}
uint64_t
TcpRlTimeBased::GenerateUuid ()
{
  static uint64_t uuid = 0;
  uuid++;
  return uuid;
}

void
TcpRlTimeBased::CreateEnv ()
{
  NS_LOG_FUNCTION (this);
  env = Create<TcpTimeStepEnv> (1234);
  std::cerr << "CreateEnv" << (env == 0) << std::endl;
  env->SetSocketUuid (TcpRlTimeBased::GenerateUuid ());

  ConnectSocketCallbacks ();
}

void
TcpRlTimeBased::ConnectSocketCallbacks ()
{
  NS_LOG_FUNCTION (this);

  bool foundSocket = false;
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<TcpL4Protocol> tcp = node->GetObject<TcpL4Protocol> ();

      ObjectVectorValue socketVec;
      tcp->GetAttribute ("SocketList", socketVec);
      NS_LOG_DEBUG ("Node: " << node->GetId () << " TCP socket num: " << socketVec.GetN ());

      uint32_t sockNum = socketVec.GetN ();
      for (uint32_t j = 0; j < sockNum; j++)
        {
          Ptr<Object> sockObj = socketVec.Get (j);
          Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase> (sockObj);
          NS_LOG_DEBUG ("Node: " << node->GetId () << " TCP Socket: " << tcpSocket);
          if (!tcpSocket)
            {
              continue;
            }

          Ptr<TcpSocketDerived> dtcpSocket = StaticCast<TcpSocketDerived> (tcpSocket);
          Ptr<TcpCongestionOps> ca = dtcpSocket->GetCongestionControlAlgorithm ();
          NS_LOG_DEBUG ("CA name: " << ca->GetName ());
          Ptr<TcpRlTimeBased> rlCa = DynamicCast<TcpRlTimeBased> (ca);
          if (rlCa == this)
            {
              NS_LOG_DEBUG ("Found TcpRl CA!");
              foundSocket = true;
              m_tcpSocket = tcpSocket;
              break;
            }
        }

      if (foundSocket)
        {
          break;
        }
    }

  NS_ASSERT_MSG (m_tcpSocket, "TCP socket was not found.");

  if (m_tcpSocket)
    {
      NS_LOG_DEBUG ("Found TCP Socket: " << m_tcpSocket);
      m_tcpSocket->TraceConnectWithoutContext ("Tx", MakeCallback (&TcpRlEnv::TxPktTrace, env));
      m_tcpSocket->TraceConnectWithoutContext ("Rx", MakeCallback (&TcpRlEnv::RxPktTrace, env));
      NS_LOG_DEBUG ("Connect socket callbacks " << m_tcpSocket->GetNode ()->GetId ());
      env->SetNodeId (m_tcpSocket->GetNode ()->GetId ());
    }
}

std::string
TcpRlTimeBased::GetName () const
{
  return "TcpRlTimeBased";
}

uint32_t
TcpRlTimeBased::GetSsThresh (Ptr<const TcpSocketState> state, uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);
  if (!m_cbConnect)
    {
      m_cbConnect = true;
      CreateEnv ();
    }

  uint32_t newSsThresh = 0;
  newSsThresh = env->GetSsThresh (state, bytesInFlight);

  return newSsThresh;
}

void
TcpRlTimeBased::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  if (!m_cbConnect)
    {
      m_cbConnect = true;
      CreateEnv ();
    }
  env->IncreaseWindow (tcb, segmentsAcked);
}

void
TcpRlTimeBased::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  NS_LOG_FUNCTION (this);
  if (!m_cbConnect)
    {
      m_cbConnect = true;
      CreateEnv ();
    }
  env->PktsAcked (tcb, segmentsAcked, rtt);
}

void
TcpRlTimeBased::CongestionStateSet (Ptr<TcpSocketState> tcb,
                                    const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this);
  if (!m_cbConnect)
    {
      m_cbConnect = true;
      CreateEnv ();
    }
  env->CongestionStateSet (tcb, newState);
}

void
TcpRlTimeBased::CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
  NS_LOG_FUNCTION (this);
  if (!m_cbConnect)
    {
      m_cbConnect = true;
      CreateEnv ();
    }
  env->CwndEvent (tcb, event);
}

Ptr<TcpCongestionOps>
TcpRlTimeBased::Fork ()
{
  return CopyObject<TcpRlTimeBased> (this);
}

} // namespace ns3
