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
 */
#pragma once
#include <cstring>
#include <sys/shm.h>
#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ns3/singleton.h"
namespace ns3 {

#define SHM_MAX_KEY_ID 16384
#define Packed __attribute__ ((__packed__))
enum CtrlInfo { LastCtrlBlock = 0, FollowCtrlBlock = 0x3 };

struct CtrlInfoBlock
{
  uint16_t ctrlInfo : 2;
  uint16_t ctrlInfoVersion : 14;
  uint16_t ctrlInfoLock;
  uint32_t freeMemOffset;
} Packed;

struct SharedMemoryCtrl
{
  uint16_t ctrlInfo : 2;
  uint16_t id : 14;
  uint32_t size;
  uint32_t offset;
} Packed;

struct SharedMemoryLockable
{
  volatile uint8_t version;
  volatile uint8_t preVersion;
  uint8_t mem[0];
} Packed;

class SharedMemoryPool : public Singleton<SharedMemoryPool>
{
public:
  uint8_t *m_memoryPoolPtr{NULL};
  CtrlInfoBlock *m_ctrlInfo;
  bool m_isCreator;
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

  void FreeMemory (void);
  void *GetMemory (uint16_t id, uint32_t size);
  void *RegisterMemory (uint16_t id, uint32_t size); //register memory with verison in pool
  void *AcquireMemory (uint16_t id);
  void *AcquireMemoryCond (uint16_t id, uint8_t mod, uint8_t res); //acquire memory if version%mod==res
  void *AcquireMemoryTarget (uint16_t id, uint8_t tar); //acquire memory if memory version is tar
  void *AcquireMemoryCondFunc (uint16_t id, bool (*cond) (uint8_t version)); //acquire memory if condition function return true
  void ReleaseMemory (uint16_t id); //release memory
  void ReleaseMemoryAndRollback (uint16_t id); //release memory and roll back version
  uint8_t GetMemoryVersion (uint16_t id);
  void IncMemoryVersion (uint16_t id);
};

} // namespace ns3