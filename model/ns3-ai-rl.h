/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Huazhong University of Science and Technology, Dian Group
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
 * Author: Pengyu Liu <eic_lpy@hust.edu.cn>
 * Author: Hao Yin <haoyin@uw.edu>
 */
#pragma once
#include "ns3/simple-ref-count.h"
#include "ns3/simulator.h"
#include "memory-pool.h"
namespace ns3
{
struct RLEmptyInfo
{
  uint8_t unused; //placeholder
} Packed;
template <typename EnvType, typename ActionType, typename SimInfoType = RLEmptyInfo>
class Ns3AIRL : public SimpleRefCount<Ns3AIRL<EnvType, ActionType, SimInfoType>>
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
  bool (*m_cond)(uint8_t version);

public:
  Ns3AIRL(void) = delete;
  Ns3AIRL(uint16_t id); //Construct and allocate memory
  ~Ns3AIRL(void);
  void SetCond(uint8_t mod, uint8_t res);
  void SetCondFunc(bool (*cond)(uint8_t version));
/**
 * \brief Get env pointer for reading. 
 *  If it is not locked, it will lock.
 * \returns env type.
 */
  EnvType *EnvGetter(void);                    //Get env pointer for reading
/**
 * \brief Get env pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version is a multiple of the mod. 
 * \returns env type.
 */
  EnvType *EnvGetterCond(void);                //Get env pointer for reading
/**
 * \brief Get env pointer for reading. 
 * If it is not locked, it will lock.
 * It only happens when its version is the specific number. 
 * \param [in] id Id of the memory to be allocated
 * \returns env type.
 */  
  EnvType *EnvGetterTarget(uint8_t tar);       //Get env pointer for reading
/**
 * \brief Get env pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version does not equal to 0. 
 * \returns env type.
 */  
  EnvType *EnvGetterCondFunc(void);            //Get env pointer for reading
/**
 * \brief get action pointer for reading. 
 *  If it is not locked, it will lock.
 *  If next version equals to present version, the present version add to 1. 
 * \returns action type.
 */  
  ActionType *ActionGetter(void);              //Get action pointer for reading
/**
 * \brief get action pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version is a multiple of the mod. 
 * \returns action type.
 */ 
  ActionType *ActionGetterCond(void);          //Get action pointer for reading
/**
 * \brief get action pointer for reading. 
 * If it is not locked, it will lock.
 * It only happens when its version is the target number. 
 * \param [in] target version number.
 * \returns env type.
 */  
  ActionType *ActionGetterTarget(uint8_t tar); //Get action pointer for reading
/**
 * \brief get action pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version does not equal to 0. 
 * \returns action type.
 */ 
  ActionType *ActionGetterCondFunc(void);      //Get action pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 *  If it is not locked, it will lock.
 * \returns information type.
 */ 
  SimInfoType *InfoGetter(void);               //Get simulation info pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version is a multiple of the mod. 
 * \returns information type.
 */ 
  SimInfoType *InfoGetterCond(void);           //Get simulation info pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 * If it is not locked, it will lock.
 * It only happens when its version is the target number. 
 * \param [in] target version number.
 * \returns information type.
 */   
  SimInfoType *InfoGetterTarget(uint8_t tar);  //Get simulation info pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version does not equal to 0. 
 * \returns information type.
 */ 
  SimInfoType *InfoGetterCondFunc(void);       //Get simulation info pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 *  If it is locked, it will unlock.
 *  It checks lock status right or not. 
 */ 
  void GetCompleted(void);                     //read completed

  EnvType *EnvSetter(void);                    //Get pointer to modify env
  EnvType *EnvSetterCond(void);                //Get pointer to modify env
  EnvType *EnvSetterTarget(uint8_t tar);       //Get pointer to modify env
  EnvType *EnvSetterCondFunc(void);            //Get pointer to modify env
  ActionType *ActionSetter(void);              //Get pointer to modify action
  ActionType *ActionSetterCond(void);          //Get pointer to modify action
  ActionType *ActionSetterTarget(uint8_t tar); //Get pointer to modify action
  ActionType *ActionSetterCondFunc(void);      //Get pointer to modify action
  SimInfoType *InfoSetter(void);               //Get pointer to modify info
  SimInfoType *InfoSetterCond(void);           //Get pointer to modify info
  SimInfoType *InfoSetterTarget(uint8_t tar);  //Get pointer to modify info
  SimInfoType *InfoSetterCondFunc(void);       //Get pointer to modify info
/**
 * \brief modification completed. 
 *  If it is not locked, it will lock.
 *  It checks lock status right or not. 
 */ 
  void SetCompleted(void);                     //modification completed
/**
 * \brief get memory version.
 *  If it is not locked, it will lock.
 *  It checks lock status right or not. 
 * \return version.
 */ 
  uint8_t GetVersion(void); //get memory version
/**
 * \brief set simulation finish. 
 *  If it is not locked, it will lock and acquire memory.
 *  If it is locked, it will unlock and release memory.
 */ 
  void SetFinish(void);     //set simulation finish
/**
 * \brief get finish identifier. 
 * \return finish flag.
 */ 
  bool GetIsFinish(void);
};

template <typename EnvType, typename ActionType, typename SimInfoType>
Ns3AIRL<EnvType, ActionType, SimInfoType>::Ns3AIRL(uint16_t id)
{
  m_mod = 2;
  m_res = 0;
  m_cond = NULL;
  m_id = id;
  m_memSize = sizeof(EnvType) + sizeof(ActionType) + sizeof(SimInfoType) + sizeof(bool);
  m_baseAddr = (uint8_t *)SharedMemoryPool::Get()->RegisterMemory(id, m_memSize);
  m_env = (EnvType *)m_baseAddr;
  m_act = (ActionType *)(m_baseAddr + sizeof(EnvType));
  m_info = (SimInfoType *)(m_baseAddr + sizeof(EnvType) + sizeof(ActionType));
  m_isFinish =
      (bool *)(m_baseAddr + sizeof(EnvType) + sizeof(ActionType) + sizeof(SimInfoType));
  Simulator::ScheduleDestroy(&Ns3AIRL<EnvType, ActionType, SimInfoType>::SetFinish, this);
}

template <typename EnvType, typename ActionType, typename SimInfoType>
Ns3AIRL<EnvType, ActionType, SimInfoType>::~Ns3AIRL(void)
{
}

template <typename EnvType, typename ActionType, typename SimInfoType>
void Ns3AIRL<EnvType, ActionType, SimInfoType>::SetCond(uint8_t mod, uint8_t res)
{
  m_mod = mod;
  m_res = res;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
void Ns3AIRL<EnvType, ActionType, SimInfoType>::SetCondFunc(bool (*cond)(uint8_t version))
{
  m_cond = cond;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
uint8_t
Ns3AIRL<EnvType, ActionType, SimInfoType>::GetVersion(void)
{
  return SharedMemoryPool::Get()->GetMemoryVersion(m_id);
}
////////////////////////////////////////////////////////////////////////////////////EnvGetter

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvGetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvGetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvGetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvGetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_env;
}
////////////////////////////////////////////////////////////////////////////////////ActGetter

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionGetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionGetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionGetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionGetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_act;
}
////////////////////////////////////////////////////////////////////////////////////InfoGetter

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoGetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoGetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoGetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoGetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_info;
}
////////////////////////////////////////////////////////////////////////////////////Get

template <typename EnvType, typename ActionType, typename SimInfoType>
void Ns3AIRL<EnvType, ActionType, SimInfoType>::GetCompleted(void)
{
  if (m_locked)
  {
    SharedMemoryPool::Get()->ReleaseMemoryAndRollback(m_id);
    m_locked = false;
  }
}
////////////////////////////////////////////////////////////////////////////////////EnvSetter

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvSetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvSetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvSetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_env;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
EnvType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::EnvSetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_env;
}
////////////////////////////////////////////////////////////////////////////////////ActSetter

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionSetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionSetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionSetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_act;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
ActionType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::ActionSetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_act;
}
////////////////////////////////////////////////////////////////////////////////////InfoSetter
template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoSetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoSetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoSetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_info;
}

template <typename EnvType, typename ActionType, typename SimInfoType>
SimInfoType *
Ns3AIRL<EnvType, ActionType, SimInfoType>::InfoSetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_info;
}
template <typename EnvType, typename ActionType, typename SimInfoType>
void Ns3AIRL<EnvType, ActionType, SimInfoType>::SetCompleted(void)
{
  if (m_locked)
  {
    SharedMemoryPool::Get()->ReleaseMemory(m_id);
    m_locked = false;
  }
}
template <typename EnvType, typename ActionType, typename SimInfoType>
void Ns3AIRL<EnvType, ActionType, SimInfoType>::SetFinish(void)
{
  __sync_bool_compare_and_swap (m_isFinish, false, true);
}
template <typename EnvType, typename ActionType, typename SimInfoType>
bool Ns3AIRL<EnvType, ActionType, SimInfoType>::GetIsFinish(void)
{
  return *m_isFinish;
}

} // namespace ns3