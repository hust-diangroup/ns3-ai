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
 */

#include "mygym.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/node-list.h"
#include "ns3/log.h"
#include <sstream>
#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MyAIEnv");

// NS_OBJECT_ENSURE_REGISTERED (MyAIEnv);

MyAIEnv::MyAIEnv(uint16_t id) : Ns3AIRL<sEnv, sAct, sInfo>(id)
{
  NS_LOG_FUNCTION(this);
  m_currentNode = 0;
  m_currentChannel = 0;
  m_collisionTh = 3;
  m_channelNum = 1;
  m_channelOccupation.clear();

  SetCond(2, 0);
}

MyAIEnv::MyAIEnv(uint16_t id, uint32_t channelNum) : Ns3AIRL<sEnv, sAct, sInfo>(id)
{
  NS_LOG_FUNCTION(this);
  m_currentNode = 0;
  m_currentChannel = 0;
  m_collisionTh = 3;
  m_channelNum = channelNum;
  m_channelOccupation.clear();

  SetCond(2, 0);
  auto info = InfoSetterCond();
  info->channelNum = channelNum;
  // SetCompleted();
}

MyAIEnv::~MyAIEnv()
{
  NS_LOG_FUNCTION(this);
}

// TypeId
// MyAIEnv::GetTypeId (void)
// {
//   static TypeId tid = TypeId ("MyAIEnv")
//     .SetParent<Ns3AIRL<sEnv, sAct>> ()
//     .SetGroupName ("MyAIEnv")
//     .AddConstructor<MyAIEnv> ()
//   ;
//   return tid;
// }

// void
// MyAIEnv::DoDispose ()
// {
//   NS_LOG_FUNCTION (this);
// }

// Ptr<OpenGymSpace>
// MyAIEnv::GetActionSpace()
// {
//   NS_LOG_FUNCTION (this);
//   Ptr<OpenGymDiscreteSpace> space = CreateObject<OpenGymDiscreteSpace> (m_channelNum);
//   NS_LOG_UNCOND ("GetActionSpace: " << space);
//   return space;
// }

// Ptr<OpenGymSpace>
// MyAIEnv::GetObservationSpace()
// {
//   NS_LOG_FUNCTION (this);
//   float low = 0.0;
//   float high = 1.0;
//   std::vector<uint32_t> shape = {m_channelNum,};
//   std::string dtype = TypeNameGet<uint32_t> ();
//   Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
//   NS_LOG_UNCOND ("GetObservationSpace: " << space);
//   return space;
// }

bool MyAIEnv::GetGameOver()
{
  NS_LOG_FUNCTION(this);
  bool isGameOver = false;

  uint32_t collisionNum = 0;
  for (auto &v : m_collisions)
    collisionNum += v;

  if (collisionNum >= m_collisionTh)
  {
    isGameOver = true;
  }
  // NS_LOG_UNCOND("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

// Ptr<OpenGymDataContainer>
// MyAIEnv::GetObservation()
// {
//   NS_LOG_FUNCTION (this);
//   std::vector<uint32_t> shape = {m_channelNum,};
//   Ptr<OpenGymBoxContainer<uint32_t> > box = CreateObject<OpenGymBoxContainer<uint32_t> >(shape);

//   for (uint32_t i = 0; i < m_channelOccupation.size(); ++i) {
//     uint32_t value = m_channelOccupation.at(i);
//     box->AddValue(value);
//   }

//   NS_LOG_UNCOND ("MyGetObservation: " << box);
//   return box;
// }

float MyAIEnv::GetReward()
{
  NS_LOG_FUNCTION(this);
  float reward = 1.0;
  if (m_channelOccupation.size() == 0)
  {
    return 0.0;
  }
  uint32_t occupied = m_channelOccupation.at(m_currentChannel);
  if (occupied == 1)
  {
    reward = -1.0;
    m_collisions.erase(m_collisions.begin());
    m_collisions.push_back(1);
  }
  else
  {
    m_collisions.erase(m_collisions.begin());
    m_collisions.push_back(0);
  }
  // NS_LOG_UNCOND("MyGetReward: " << reward);
  return reward;
}

// std::string
// MyAIEnv::GetExtraInfo()
// {
//   NS_LOG_FUNCTION (this);
//   std::string myInfo = "info";
//   NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
//   return myInfo;
// }

// bool
// MyAIEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
// {
//   NS_LOG_FUNCTION (this);
//   Ptr<OpenGymDiscreteContainer> discrete = DynamicCast<OpenGymDiscreteContainer>(action);
//   uint32_t nextChannel = discrete->GetValue();
//   m_currentChannel = nextChannel;

//   NS_LOG_UNCOND ("Current Channel: " << m_currentChannel);
//   return true;
// }

void MyAIEnv::CollectChannelOccupation(uint32_t chanId, uint32_t occupied)
{
  NS_LOG_FUNCTION(this);
  m_channelOccupation.push_back(occupied);
}

bool MyAIEnv::CheckIfReady()
{
  NS_LOG_FUNCTION(this);
  return m_channelOccupation.size() == m_channelNum;
}

void MyAIEnv::ClearObs()
{
  NS_LOG_FUNCTION(this);
  m_channelOccupation.clear();
}

void MyAIEnv::PerformCca(Ptr<MyAIEnv> entity, uint32_t channelId, Ptr<const SpectrumValue> avgPowerSpectralDensity)
{
  double power = Integral(*(avgPowerSpectralDensity));
  double powerDbW = 10 * std::log10(power);
  double threshold = -60;
  uint32_t busy = powerDbW > threshold;
  // NS_LOG_UNCOND("Channel: " << channelId << " CCA: " << busy << " RxPower: " << powerDbW);

  entity->CollectChannelOccupation(channelId, busy);

  if (entity->CheckIfReady())
  {
    // entity->Notify();
    auto env = entity->EnvSetterCond();
    env->reward = entity->GetReward();
    env->done = entity->GetGameOver();
    env->channNum = entity->m_channelOccupation.size();
    for (uint32_t i = 0; i < entity->m_channelOccupation.size(); ++i)
    {
      env->channelOccupation[i] = entity->m_channelOccupation.at(i);
    }
    entity->SetCompleted();
    entity->ClearObs();
    auto act = entity->ActionGetterCond();
    entity->m_currentChannel = act->nextChannel;
    entity->GetCompleted();
  }
}

} // namespace ns3