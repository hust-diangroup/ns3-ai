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
struct DLEmptyInfo
{
  uint8_t unused; //placeholder
} Packed;
template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType = DLEmptyInfo>
class Ns3AIDL : public SimpleRefCount<Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>>
{
protected:
  uint32_t m_memSize;
  uint8_t *m_baseAddr;
  FeatureType *m_feature;
  PredictedType *m_predicted;
  TargetType *m_target;
  SimInfoType *m_info;
  uint16_t m_id;
  bool m_locked{false};
  bool *m_isFinish;

  uint8_t m_mod;
  uint8_t m_res;
  bool (*m_cond)(uint8_t version);

public:
  Ns3AIDL(void) = delete;
  Ns3AIDL(uint16_t id); //Construct and allocate memory
  ~Ns3AIDL(void);
  void SetCond(uint8_t mod, uint8_t res);
  void SetCondFunc(bool (*cond)(uint8_t version));
/**
 * \brief Get feature pointer for reading. 
 *  If it is not locked, it will lock.
 * \returns feature type.
 */
  FeatureType *FeatureGetter(void);                  //Get feature pointer for reading
/**
 * \brief Get feature pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version is a multiple of the mod. 
 * \returns feature type.
 */
  FeatureType *FeatureGetterCond(void);              //Get feature pointer for reading
/**
 * \brief Get feature pointer for reading. 
 * If it is not locked, it will lock.
 * It only happens when its version is the specific number. 
 * \param [in] id Id of the memory to be allocated
 * \returns feature type.
 */ 
  FeatureType *FeatureGetterTarget(uint8_t tar);     //Get feature pointer for reading
/**
 * \brief Get feature pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version does not equal to 0. 
 * \returns feature type.
 */  
  FeatureType *FeatureGetterCondFunc(void);          //Get feature pointer for reading
/**
 * \brief get predicted pointer for reading. 
 *  If it is not locked, it will lock.
 *  If next version equals to present version, the present version add to 1. 
 * \returns predicted type.
 */  
  PredictedType *PredictedGetter(void);              //Get predicted pointer for reading
/**
 * \brief get predicted pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version is a multiple of the mod. 
 * \returns predicted type.
 */ 
  PredictedType *PredictedGetterCond(void);          //Get predicted pointer for reading
/**
 * \brief get predicted pointer for reading. 
 * If it is not locked, it will lock.
 * It only happens when its version is the target number. 
 * \param [in] target version number.
 * \returns predicted type.
 */  
  PredictedType *PredictedGetterTarget(uint8_t tar); //Get predicted pointer for reading
/**
 * \brief get predicted pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version does not equal to 0. 
 * \returns predicted type.
 */ 
  PredictedType *PredictedGetterCondFunc(void);      //Get predicted pointer for reading
/**
 * \brief target pointer for reading. 
 *  If it is not locked, it will lock.
 * \returns target type.
 */ 
  TargetType *TargetGetter(void);                    //Get target pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version is a multiple of the mod. 
 * \returns information type.
 */  
  TargetType *TargetGetterCond(void);                //Get target pointer for reading
/**
 * \brief get target pointer for reading. 
 * If it is not locked, it will lock.
 * It only happens when its version is the target number. 
 * \param [in] target version number.
 * \returns target type.
 */   
  TargetType *TargetGetterTarget(uint8_t tar);       //Get target pointer for reading
/**
 * \brief get target pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version does not equal to 0. 
 * \returns target type.
 */   
  TargetType *TargetGetterCondFunc(void);            //Get target pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 *  If it is not locked, it will lock.
 * \returns information type.
 */ 
  SimInfoType *InfoGetter(void);                     //Get simulation info pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 *  If it is not locked, it will lock.
 *  It only happens when its version is a multiple of the mod. 
 * \returns information type.
 */ 
  SimInfoType *InfoGetterCond(void);                 //Get simulation info pointer for reading
/**
 * \brief get simulation info pointer for reading. 
 * If it is not locked, it will lock.
 * It only happens when its version is the target number. 
 * \param [in] target version number.
 * \returns information type.
 */  
  SimInfoType *InfoGetterTarget(uint8_t tar);        //Get simulation info pointer for reading
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

  FeatureType *FeatureSetter(void);                  //Get pointer to modify feature
  FeatureType *FeatureSetterCond(void);              //Get pointer to modify feature
  FeatureType *FeatureSetterTarget(uint8_t tar);     //Get pointer to modify feature
  FeatureType *FeatureSetterCondFunc(void);          //Get pointer to modify feature
  PredictedType *PredictedSetter(void);              //Get pointer to modify predicted
  PredictedType *PredictedSetterCond(void);          //Get pointer to modify predicted
  PredictedType *PredictedSetterTarget(uint8_t tar); //Get pointer to modify predicted
  PredictedType *PredictedSetterCondFunc(void);      //Get pointer to modify predicted
  TargetType *TargetSetter(void);                    //Get pointer to modify target
  TargetType *TargetSetterCond(void);                //Get pointer to modify target
  TargetType *TargetSetterTarget(uint8_t tar);       //Get pointer to modify target
  TargetType *TargetSetterCondFunc(void);            //Get pointer to modify target
  SimInfoType *InfoSetter(void);                     //Get pointer to modify info
  SimInfoType *InfoSetterCond(void);                 //Get pointer to modify info
  SimInfoType *InfoSetterTarget(uint8_t tar);        //Get pointer to modify info
  SimInfoType *InfoSetterCondFunc(void);             //Get pointer to modify info
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

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::Ns3AIDL(uint16_t id)
{
  m_mod = 2;
  m_res = 0;
  m_cond = NULL;
  m_id = id;
  m_memSize = sizeof(FeatureType) + sizeof(PredictedType) + sizeof(TargetType) + sizeof(SimInfoType) + sizeof(bool);
  m_baseAddr = (uint8_t *)SharedMemoryPool::Get()->RegisterMemory(id, m_memSize);
  m_feature = (FeatureType *)m_baseAddr;
  m_predicted = (PredictedType *)(m_baseAddr + sizeof(FeatureType));
  m_target = (TargetType *)(m_baseAddr + sizeof(FeatureType) + sizeof(PredictedType));
  m_info = (SimInfoType *)(m_baseAddr + sizeof(FeatureType) + sizeof(PredictedType) + sizeof(TargetType));
  m_isFinish =
      (bool *)(m_baseAddr + sizeof(FeatureType) + sizeof(PredictedType) + sizeof(TargetType) + sizeof(SimInfoType));
  Simulator::ScheduleDestroy(&Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::SetFinish, this);
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::~Ns3AIDL(void)
{
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
void Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::SetCond(uint8_t mod, uint8_t res)
{
  m_mod = mod;
  m_res = res;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
void Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::SetCondFunc(bool (*cond)(uint8_t version))
{
  m_cond = cond;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
uint8_t
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::GetVersion(void)
{
  return SharedMemoryPool::Get()->GetMemoryVersion(m_id);
}
////////////////////////////////////////////////////////////////////////////////////FeatureGetter

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureGetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_feature;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureGetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_feature;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureGetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_feature;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureGetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_feature;
}
////////////////////////////////////////////////////////////////////////////////////PredictedGetter

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedGetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_predicted;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedGetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_predicted;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedGetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_predicted;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedGetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_predicted;
}

////////////////////////////////////////////////////////////////////////////////////TargetGetter

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetGetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_target;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetGetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_target;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetGetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_target;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetGetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_target;
}
////////////////////////////////////////////////////////////////////////////////////InfoGetter

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoGetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_info;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoGetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_info;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoGetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_info;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoGetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_info;
}
////////////////////////////////////////////////////////////////////////////////////Get

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
void Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::GetCompleted(void)
{
  if (m_locked)
  {
    SharedMemoryPool::Get()->ReleaseMemoryAndRollback(m_id);
    m_locked = false;
  }
}
////////////////////////////////////////////////////////////////////////////////////FeatureSetter

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureSetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_feature;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureSetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_feature;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureSetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_feature;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
FeatureType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::FeatureSetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_feature;
}
////////////////////////////////////////////////////////////////////////////////////PredictedSetter

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedSetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_predicted;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedSetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_predicted;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedSetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_predicted;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
PredictedType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::PredictedSetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_predicted;
}

////////////////////////////////////////////////////////////////////////////////////TargetSetter

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetSetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_target;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetSetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_target;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetSetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_target;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
TargetType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::TargetSetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_target;
}
////////////////////////////////////////////////////////////////////////////////////InfoSetter
template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoSetter(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemory(m_id);
    m_locked = true;
  }
  return m_info;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoSetterCond(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCond(m_id, m_mod, m_res);
    m_locked = true;
  }
  return m_info;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoSetterTarget(uint8_t tar)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryTarget(m_id, tar);
    m_locked = true;
  }
  return m_info;
}

template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
SimInfoType *
Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::InfoSetterCondFunc(void)
{
  if (!m_locked)
  {
    SharedMemoryPool::Get()->AcquireMemoryCondFunc(m_id, m_cond);
    m_locked = true;
  }
  return m_info;
}
template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
void Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::SetCompleted(void)
{
  if (m_locked)
  {
    SharedMemoryPool::Get()->ReleaseMemory(m_id);
    m_locked = false;
  }
}
template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
void Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::SetFinish(void)
{
  __sync_bool_compare_and_swap (m_isFinish, false, true);
}
template <typename FeatureType, typename PredictedType, typename TargetType, typename SimInfoType>
bool Ns3AIDL<FeatureType, PredictedType, TargetType, SimInfoType>::GetIsFinish(void)
{
  return *m_isFinish;
}

} // namespace ns3