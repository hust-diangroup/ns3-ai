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
#include <cstring>
#include <sys/shm.h>
#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ns3/singleton.h"
/**
 * \file
 * \ingroup tracing
 * ns3::memory-pool declaration.
 */
namespace ns3 {
/**
 * \ingroup tracing
 * \brief Achieve function of 
 * getting the memory 
 * freeing the memory 
 * registering the memory
 * aquiring the memory
 * releasing the memory
 * and so on
 */
#define SHM_MAX_KEY_ID 16384
#define Packed __attribute__ ((__packed__))
enum CtrlInfo { LastCtrlBlock = 0, FollowCtrlBlock = 0x3 };

struct CtrlInfoBlock
{
  /** control information */
  volatile uint16_t ctrlInfo : 2;
  /** control information version */
  volatile uint16_t ctrlInfoVersion : 14;
  /** control information lock */
  volatile uint16_t ctrlInfoLock;
  /** free memory offset */
  volatile uint32_t freeMemOffset;
} Packed;

struct SharedMemoryCtrl
{
  /** control information */
  volatile uint16_t ctrlInfo : 2;
  /** related to the memory address */
  volatile uint16_t id : 14;
  /** memory size */
  volatile uint32_t size;
  /** memory offset */
  volatile uint32_t offset;
} Packed;

struct SharedMemoryLockable
{
  /** present version */
  volatile uint8_t version;
  /** next version */
  volatile uint8_t nextVersion;
  /** memory base address */
  uint8_t mem[0];
} Packed;

class SharedMemoryPool : public Singleton<SharedMemoryPool>
{
public:
  uint8_t *m_memoryPoolPtr{NULL};
  CtrlInfoBlock *m_ctrlInfo;
  // bool m_isCreator;
  key_t m_shmid;
  struct shmid_ds m_shmds;
  uint32_t m_memoryPoolSize;
  uint32_t m_memoryKey;
  uint32_t m_configLen;
  uint32_t m_ctrlBlockSize;
  uint16_t m_currentVersion;
  SharedMemoryCtrl *m_curCtrlInfo;

  SharedMemoryCtrl *m_memoryCtrlInfo[SHM_MAX_KEY_ID];
  SharedMemoryLockable *m_memoryLocker[SHM_MAX_KEY_ID];
  void CtrlInfoLock (void);
  void CtrlInfoUnlock (void);

public:
  SharedMemoryPool (void);
  ~SharedMemoryPool (void);
/**
 * \brief After all processes are complete, free up the memory pool.
 */
  void FreeMemory (void);
/**
 * \brief Get the memory of the size of the parameter size corresponding to the parameter id. 
 * If the memory does not exist, the memory will be allocated.
 * \param [in] id Id of the memory to be allocated
 * \param [in] size Size of the memory to be allocated
 * \returns Pointer to the memory to be allocated.
 */
  void *GetMemory (uint16_t id, uint32_t size);
/**
 * \brief Get the memory of the size of the parameter size corresponding to the parameter id. 
 * If the memory does not exist, the memory will be allocated.
 *  The memory has a version information for synchronization.
 * \param [in] id Id of the memory to be allocated
 * \param [in] size Size of the memory to be allocated
 * \returns Pointer to the memory to be allocated.
 */
  void *RegisterMemory (uint16_t id, uint32_t size); //register memory with verison in pool
/**
 * \brief Lock the memory with id as parameter id. Memory should be registered first.
 * \param [in] id Id of the memory to be Locked
 * \returns Pointer to the memory with id as parameter id.
 */
  void *AcquireMemory (uint16_t id);
/**
 * \brief Acquire memory if version%mod==res.
 * \param [in] id Id of the memory to be Locked
 * \param [in] mod get the remainder relative to MOD
 * \param [in] the remainder we require
 * \returns Pointer to the memory with id as parameter id.
 */
  void *AcquireMemoryCond (uint16_t id, uint8_t mod, uint8_t res); //acquire memory if version%mod==res
/**
 * \brief acquire memory if memory version is tar.
 * \param [in] id Id of the memory to be Locked
 * \param [in] tar TAR of the memory to be allocated
 * \returns Pointer to the memory with id as parameter id.
 */
  void *AcquireMemoryTarget (uint16_t id, uint8_t tar); //acquire memory if memory version is tar
/**
 * \brief acquire memory if condition function return true.
 * \param [in] id Id of the memory to be Locked
 * \param [in] cond Function to judge if condition is true
 * \returns Pointer to the memory with id as parameter id.
 */
  void *AcquireMemoryCondFunc (uint16_t id, bool (*cond) (uint8_t version)); //acquire memory if condition function return true
/**
 * \brief Release the lock of memory with id as parameter id. Memory should be registered first.
 * \param [in] id Id of the memory to be Unlocked
 */
  void ReleaseMemory (uint16_t id); //release memory
/**
 * \brief Release the lock of memory with id as parameter id. Memory should be registered first.
 * \param [in] id Id of the memory to be Unlocked
 * \returns Pointer to the memory with id as parameter id.
 */
  void ReleaseMemoryAndRollback (uint16_t id); //release memory and roll back version
/**
 * \brief Get the version of memory with id as parameter id. Memory should be registered first.
 * \param [in] id Id of the memory 
 * \returns Version of the memory.
 */
  uint8_t GetMemoryVersion (uint16_t id);
  void IncMemoryVersion (uint16_t id);
};

} // namespace ns3