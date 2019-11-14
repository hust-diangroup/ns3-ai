/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Pengyu Liu <eic_lpy@hust.edu.cn>
 */
#pragma once
#include "ns3/simple-ref-count.h"
#include "memory-pool.h"
namespace ns3 {
struct RLBaseInfo
{
  bool isFinish;
};
template <typename EnvType, typename ActionType, typename SimInfoType = RLBaseInfo>
class ShmRL : public SimpleRefCount<ShmRL<EnvType, ActionType, SimInfoType>>
{
protected:
  uint8_t *m_baseAddr;
  EnvType *m_env;
  ActionType *m_act;
  SimInfoType *m_info;
  uint16_t m_id;
  bool m_locked{false};

public:
  ShmRL (void) = delete;
  ShmRL (uint16_t id); //Construct and allocate memory
  ~ShmRL (void);
  EnvType *EnvGetter (void); //Get env pointer for reading
  ActionType *ActionGetter (void); //Get action pointer for reading
  SimInfoType *InfoGetter (void); //Get simulation info pointer for reading
  void GetCompleted (void); //read completed

  EnvType *EnvSetter (void); //Get pointer to modify env
  ActionType *ActionSetter (void); //Get pointer to modify action
  SimInfoType *InfoSetter (void); //Get pointer to modify info
  void SetCompleted (void); //modification completed
  uint8_t GetVersion (void);
};

template <typename EnvType, typename ActionType, typename SimInfoType>
ShmRL<EnvType, ActionType, SimInfoType>::ShmRL (uint16_t id)
{
  m_id = id;
  m_baseAddr = (uint8_t *) SharedMemoryPool::Get ()->RegisterMemory (
      id, sizeof (EnvType) + sizeof (ActionType) + sizeof (RLBaseInfo));
  m_env = (EnvType *) m_baseAddr;
  m_act = (ActionType *) (m_baseAddr + sizeof (EnvType));
  m_info = (RLBaseInfo *) (m_baseAddr + sizeof (EnvType) + sizeof (ActionType));
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ShmRL<EnvType, ActionType, SimInfoType>::~ShmRL (void)
{
}

template <typename EnvType, typename ActionType, typename SimInfoType>
uint8_t
ShmRL<EnvType, ActionType, SimInfoType>::GetVersion (void)
{
  return SharedMemoryPool::Get ()->GetMemoryVersion (m_id);
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvGetter (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemory (m_id);
      m_locked = true;
    }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionGetter (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemory (m_id);
      m_locked = true;
    }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoGetter (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemory (m_id);
      m_locked = true;
    }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
void
ShmRL<EnvType, ActionType, SimInfoType>::GetCompleted (void)
{
  if (m_locked)
    {
      SharedMemoryPool::Get ()->ReleaseMemoryAndRollback (m_id);
      m_locked = false;
    }
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvSetter (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemory (m_id);
      m_locked = true;
    }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionSetter (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemory (m_id);
      m_locked = true;
    }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoSetter (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemory (m_id);
      m_locked = true;
    }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
void
ShmRL<EnvType, ActionType, SimInfoType>::SetCompleted (void)
{
  if (m_locked)
    {
      SharedMemoryPool::Get ()->ReleaseMemory (m_id);
      m_locked = false;
    }
}

} // namespace ns3