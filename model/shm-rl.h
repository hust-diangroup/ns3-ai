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
struct RLEmptyInfo
{
};
template <typename EnvType, typename ActionType, typename SimInfoType = RLEmptyInfo>
class ShmRL : public SimpleRefCount<ShmRL<EnvType, ActionType, SimInfoType>>
{
protected:
  uint32_t m_memSize;
  uint8_t *m_baseAddr;
  EnvType *m_env;
  ActionType *m_act;
  SimInfoType *m_info;
  uint16_t m_id;
  bool m_locked{false};
  bool *m_isFinish;

  uint8_t m_mod;
  uint8_t m_res;
  bool (*m_cond) (uint8_t version);

public:
  ShmRL (void) = delete;
  ShmRL (uint16_t id); //Construct and allocate memory
  ~ShmRL (void);
  void SetCond (uint8_t mod, uint8_t res);
  void SetCondFunc (bool (*cond) (uint8_t version));
  EnvType *EnvGetter (void); //Get env pointer for reading
  EnvType *EnvGetterCond (void); //Get env pointer for reading
  EnvType *EnvGetterTarget (uint8_t tar); //Get env pointer for reading
  EnvType *EnvGetterCondFunc (void); //Get env pointer for reading
  ActionType *ActionGetter (void); //Get action pointer for reading
  ActionType *ActionGetterCond (void); //Get action pointer for reading
  ActionType *ActionGetterTarget (uint8_t tar); //Get action pointer for reading
  ActionType *ActionGetterCondFunc (void); //Get action pointer for reading
  SimInfoType *InfoGetter (void); //Get simulation info pointer for reading
  SimInfoType *InfoGetterCond (void); //Get simulation info pointer for reading
  SimInfoType *InfoGetterTarget (uint8_t tar); //Get simulation info pointer for reading
  SimInfoType *InfoGetterCondFunc (void); //Get simulation info pointer for reading
  void GetCompleted (void); //read completed

  EnvType *EnvSetter (void); //Get pointer to modify env
  EnvType *EnvSetterCond (void); //Get pointer to modify env
  EnvType *EnvSetterTarget (uint8_t tar); //Get pointer to modify env
  EnvType *EnvSetterCondFunc (void); //Get pointer to modify env
  ActionType *ActionSetter (void); //Get pointer to modify action
  ActionType *ActionSetterCond (void); //Get pointer to modify action
  ActionType *ActionSetterTarget (uint8_t tar); //Get pointer to modify action
  ActionType *ActionSetterCondFunc (void); //Get pointer to modify action
  SimInfoType *InfoSetter (void); //Get pointer to modify info
  SimInfoType *InfoSetterCond (void); //Get pointer to modify info
  SimInfoType *InfoSetterTarget (uint8_t tar); //Get pointer to modify info
  SimInfoType *InfoSetterCondFunc (void); //Get pointer to modify info
  void SetCompleted (void); //modification completed

  uint8_t GetVersion (void); //get memory version
  void SetFinish (void); //set simulation finish
  bool GetIsFinish (void);
};

template <typename EnvType, typename ActionType, typename SimInfoType>
ShmRL<EnvType, ActionType, SimInfoType>::ShmRL (uint16_t id)
{
  m_mod = 2;
  m_res = 0;
  m_cond = NULL;
  m_id = id;
  m_memSize = sizeof (EnvType) + sizeof (ActionType) + sizeof (SimInfoType) + sizeof (bool);
  m_baseAddr = (uint8_t *) SharedMemoryPool::Get ()->RegisterMemory (id, m_memSize);
  m_env = (EnvType *) m_baseAddr;
  m_act = (ActionType *) (m_baseAddr + sizeof (EnvType));
  m_info = (SimInfoType *) (m_baseAddr + sizeof (EnvType) + sizeof (ActionType));
  m_isFinish =
      (bool *) (m_baseAddr + sizeof (EnvType) + sizeof (ActionType) + sizeof (SimInfoType));
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ShmRL<EnvType, ActionType, SimInfoType>::~ShmRL (void)
{
}

template <typename EnvType, typename ActionType, typename SimInfoType>
void
ShmRL<EnvType, ActionType, SimInfoType>::SetCond (uint8_t mod, uint8_t res)
{
  m_mod = mod;
  m_res = res;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
void
ShmRL<EnvType, ActionType, SimInfoType>::SetCondFunc (bool (*cond) (uint8_t version))
{
  m_cond = cond;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
uint8_t
ShmRL<EnvType, ActionType, SimInfoType>::GetVersion (void)
{
  return SharedMemoryPool::Get ()->GetMemoryVersion (m_id);
}
////////////////////////////////////////////////////////////////////////////////////EnvGetter

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
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvGetterCond (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCond (m_id, m_mod, m_res);
      m_locked = true;
    }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvGetterTarget (uint8_t tar)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryTarget (m_id, tar);
      m_locked = true;
    }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvGetterCondFunc (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCondFunc (m_id, m_cond);
      m_locked = true;
    }
  return m_env;
}
////////////////////////////////////////////////////////////////////////////////////ActGetter

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
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionGetterCond (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCond (m_id, m_mod, m_res);
      m_locked = true;
    }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionGetterTarget (uint8_t tar)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryTarget (m_id, tar);
      m_locked = true;
    }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionGetterCondFunc (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCondFunc (m_id, m_cond);
      m_locked = true;
    }
  return m_act;
}
////////////////////////////////////////////////////////////////////////////////////InfoGetter

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
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoGetterCond (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCond (m_id, m_mod, m_res);
      m_locked = true;
    }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoGetterTarget (uint8_t tar)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryTarget (m_id, tar);
      m_locked = true;
    }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoGetterCondFunc (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCondFunc (m_id, m_cond);
      m_locked = true;
    }
  return m_info;
}
////////////////////////////////////////////////////////////////////////////////////Get

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
////////////////////////////////////////////////////////////////////////////////////EnvSetter

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
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvSetterCond (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCond (m_id, m_mod, m_res);
      m_locked = true;
    }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvSetterTarget (uint8_t tar)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryTarget (m_id, tar);
      m_locked = true;
    }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
ShmRL<EnvType, ActionType, SimInfoType>::EnvSetterCondFunc (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCondFunc (m_id, m_cond);
      m_locked = true;
    }
  return m_env;
}
////////////////////////////////////////////////////////////////////////////////////ActSetter

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
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionSetterCond (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCond (m_id, m_mod, m_res);
      m_locked = true;
    }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionSetterTarget (uint8_t tar)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryTarget (m_id, tar);
      m_locked = true;
    }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
ShmRL<EnvType, ActionType, SimInfoType>::ActionSetterCondFunc (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCondFunc (m_id, m_cond);
      m_locked = true;
    }
  return m_act;
}
////////////////////////////////////////////////////////////////////////////////////InfoSetter
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
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoSetterCond (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCond (m_id, m_mod, m_res);
      m_locked = true;
    }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoSetterTarget (uint8_t tar)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryTarget (m_id, tar);
      m_locked = true;
    }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
ShmRL<EnvType, ActionType, SimInfoType>::InfoSetterCondFunc (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemoryCondFunc (m_id, m_cond);
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
template <typename EnvType, typename ActionType, typename SimInfoType>
void
ShmRL<EnvType, ActionType, SimInfoType>::SetFinish (void)
{
  if (!m_locked)
    {
      SharedMemoryPool::Get ()->AcquireMemory (m_id);
      m_locked = true;
    }
  *m_isFinish = true;
  if (m_locked)
    {
      SharedMemoryPool::Get ()->ReleaseMemory (m_id);
      m_locked = false;
    }
}
template <typename EnvType, typename ActionType, typename SimInfoType>
bool
ShmRL<EnvType, ActionType, SimInfoType>::GetIsFinish (void)
{
  return *m_isFinish;
}

} // namespace ns3