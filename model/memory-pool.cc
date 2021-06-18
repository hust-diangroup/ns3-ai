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
#include <unistd.h>
#include <sys/ipc.h>
#include <sstream>
#include "ns3/global-value.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "memory-pool.h"

#if defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__)
#define ShmYield() __asm__ __volatile__("pause")
#elif defined(__ia64__) || defined(__ia64)
#define ShmYield() __asm__ __volatile__("hint @pause")
#elif defined(__arm__)
#define ShmYield() __asm__ __volatile__("yield")
#endif
#endif

#if !defined(ShmYield)
#define ShmYield() usleep(0)
#endif

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("ShmPool");
GlobalValue gSharedMemoryPoolSize =
    GlobalValue ("SharedMemoryPoolSize", "Shared Memory Pool Size", UintegerValue (4096),
                 MakeUintegerChecker<uint32_t> ());
GlobalValue gSharedMemoryKey = GlobalValue ("SharedMemoryKey", "Shared Memory Key",
                                            UintegerValue (1234), MakeUintegerChecker<uint32_t> ());

SharedMemoryPool::SharedMemoryPool (void)
{
  NS_LOG_FUNCTION (this);
  UintegerValue uv;
  gSharedMemoryPoolSize.GetValue (uv);
  m_memoryPoolSize = uv.Get ();
  gSharedMemoryKey.GetValue (uv);
  m_memoryKey = uv.Get ();
  NS_LOG_INFO ("Key: " << m_memoryKey << " Size: " << m_memoryPoolSize);

  memset (m_memoryCtrlInfo, 0, sizeof (m_memoryCtrlInfo));
  memset (m_memoryLocker, 0, sizeof (m_memoryLocker));
  m_ctrlBlockSize = sizeof (SharedMemoryCtrl);
  m_configLen = sizeof (CtrlInfoBlock);

  m_shmid = shmget (m_memoryKey, m_memoryPoolSize, 0666 | IPC_CREAT);
  NS_ASSERT_MSG (m_shmid >= 0, "Cannot alloc shared memory(shmid<0)");

  m_memoryPoolPtr = (uint8_t *) shmat (m_shmid, NULL, 0);
  NS_ASSERT_MSG (m_memoryPoolPtr > (void *)0, "Cannot alloc shared memory(ptr error)");

  shmctl (m_shmid, IPC_STAT, &m_shmds);
  // m_isCreator = getpid () == m_shmds.shm_cpid;
  // if (m_isCreator)
  //   {
  //     NS_LOG_INFO ("Creator");
  //     memset (m_memoryPoolPtr, 0, m_memoryPoolSize);
  //   }
  // else
  //   {
  //     NS_LOG_INFO ("User");
  //     sleep (1);
  //   }

  m_ctrlInfo = (CtrlInfoBlock *) (m_memoryPoolPtr + m_memoryPoolSize - m_configLen);
  m_curCtrlInfo = (SharedMemoryCtrl *) m_ctrlInfo;
  m_currentVersion = 0;
  CtrlInfoLock ();
  if (m_ctrlInfo->ctrlInfoVersion == m_currentVersion)
    {
      CtrlInfoUnlock ();
      return;
    }
  while (m_curCtrlInfo->ctrlInfo == FollowCtrlBlock)
    {
      --m_curCtrlInfo;
      NS_ASSERT_MSG (m_memoryCtrlInfo[m_curCtrlInfo->id] == 0, "Id has been used");
      m_memoryCtrlInfo[m_curCtrlInfo->id] = m_curCtrlInfo;
    }
  m_currentVersion = m_ctrlInfo->ctrlInfoVersion;
  CtrlInfoUnlock ();
}

SharedMemoryPool::~SharedMemoryPool (void)
{
  FreeMemory ();
}

void
SharedMemoryPool::FreeMemory (void)
{
  NS_LOG_FUNCTION (this);
  // if (!m_isCreator || m_memoryPoolPtr == NULL)
  //   return;
  // shmctl (m_shmid, IPC_STAT, &m_shmds);
  // while (m_shmds.shm_nattch != 1)
  //   shmctl (m_shmid, IPC_STAT, &m_shmds);
  // shmctl (m_shmid, IPC_RMID, 0);
  m_memoryPoolPtr = NULL;
}

void
SharedMemoryPool::CtrlInfoLock (void)
{
  while (!__sync_lock_test_and_set(&m_ctrlInfo->ctrlInfoLock, 0xffff))
    ShmYield();
}
void
SharedMemoryPool::CtrlInfoUnlock (void)
{
  __sync_lock_release(&m_ctrlInfo->ctrlInfoLock);
}

void *
SharedMemoryPool::GetMemory (uint16_t id, uint32_t size)
{
  NS_LOG_FUNCTION (this << "ID: " << id << " Size: " << size);
  NS_ASSERT_MSG (id < SHM_MAX_KEY_ID, "Id out of range");
  CtrlInfoLock ();
  if (m_ctrlInfo->ctrlInfoVersion != m_currentVersion)
    {
      while (m_curCtrlInfo->ctrlInfo == FollowCtrlBlock)
        {
          --m_curCtrlInfo;
          if (m_memoryCtrlInfo[m_curCtrlInfo->id] != 0)
            {
              std::stringstream errMsg;
              errMsg << "Id " << m_curCtrlInfo->id << " has been used" << std::endl;
              NS_ABORT_MSG (errMsg.str());
            }
          m_memoryCtrlInfo[m_curCtrlInfo->id] = m_curCtrlInfo;
        }
      m_currentVersion = m_ctrlInfo->ctrlInfoVersion;
    }
  if (m_memoryCtrlInfo[id] != 0)
    {
      NS_ASSERT_MSG (size == m_memoryCtrlInfo[id]->size, "Size of memory error");
      CtrlInfoUnlock ();
      return m_memoryPoolPtr + m_memoryCtrlInfo[id]->offset;
    };

  NS_ASSERT_MSG (m_ctrlInfo->freeMemOffset + size <
                     m_memoryPoolSize - m_configLen -
                         (m_ctrlInfo->ctrlInfoVersion + 1) * m_ctrlBlockSize,
                 "Memory pool full");
  m_curCtrlInfo->ctrlInfo = FollowCtrlBlock;
  --m_curCtrlInfo;
  m_curCtrlInfo->ctrlInfo = LastCtrlBlock;
  m_curCtrlInfo->id = id;
  m_curCtrlInfo->size = size;
  m_curCtrlInfo->offset = m_ctrlInfo->freeMemOffset;
  m_memoryCtrlInfo[id] = m_curCtrlInfo;

  ++m_ctrlInfo->ctrlInfoVersion;
  m_currentVersion = m_ctrlInfo->ctrlInfoVersion;

  void *ret = m_memoryPoolPtr + m_ctrlInfo->freeMemOffset;
  m_ctrlInfo->freeMemOffset += size;
  CtrlInfoUnlock ();
  return ret;
}

void *
SharedMemoryPool::RegisterMemory (uint16_t id, uint32_t size)
{
  NS_LOG_FUNCTION (this << "ID: " << id << " Size: " << size);
  m_memoryLocker[id] =
      (SharedMemoryLockable *) GetMemory (id, size + sizeof (SharedMemoryLockable));
  return m_memoryLocker[id]->mem;
}

void *SharedMemoryPool::AcquireMemory (uint16_t id) //Should register first
{
  NS_LOG_FUNCTION (this << "ID: " << id);
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (!__sync_bool_compare_and_swap(&info->nextVersion, info->version, info->version + (uint8_t)1))
    ShmYield();
  return info->mem;
}

void *
SharedMemoryPool::AcquireMemoryCond (uint16_t id, uint8_t mod, uint8_t res)
{
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (info->version % mod != res)
    ShmYield();
  while (!__sync_bool_compare_and_swap(&info->nextVersion, info->version, info->version + (uint8_t)1))
    ShmYield();
  return info->mem;
}

void *
SharedMemoryPool::AcquireMemoryTarget (uint16_t id, uint8_t tar)
{
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (info->version != tar)
    ShmYield();
  while (!__sync_bool_compare_and_swap(&info->nextVersion, info->version, info->version + (uint8_t)1))
    ShmYield();
  return info->mem;
}

void *
SharedMemoryPool::AcquireMemoryCondFunc (uint16_t id, bool (*cond) (uint8_t version))
{
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (!cond(info->version))
    ShmYield();
  while (!__sync_bool_compare_and_swap(&info->nextVersion, info->version, info->version + (uint8_t)1))
    ShmYield();
  return info->mem;
}

void SharedMemoryPool::ReleaseMemory (uint16_t id) //Should register first
{
  NS_LOG_FUNCTION (this << "ID: " << id);
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (!__sync_bool_compare_and_swap(&info->version, info->nextVersion - (uint8_t)1, info->nextVersion))
    ShmYield();
}

void
SharedMemoryPool::ReleaseMemoryAndRollback (uint16_t id)
{
  NS_LOG_FUNCTION (this << "ID: " << id);
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (!__sync_bool_compare_and_swap(&info->nextVersion, info->version + (uint8_t)1, info->version))
    ShmYield();
}

uint8_t
SharedMemoryPool::GetMemoryVersion (uint16_t id)
{
  return m_memoryLocker[id]->version;
}

void
SharedMemoryPool::IncMemoryVersion (uint16_t id)
{
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (!__sync_bool_compare_and_swap(&info->nextVersion, info->version, info->version + (uint8_t)1))
    ShmYield();
  while (!__sync_bool_compare_and_swap(&info->version, info->nextVersion - (uint8_t)1, info->nextVersion))
    ShmYield();
}

} // namespace ns3
