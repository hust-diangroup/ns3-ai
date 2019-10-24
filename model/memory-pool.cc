/* -*- Mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <unistd.h>
#include <sys/ipc.h>
#include "ns3/global-value.h"
#include "ns3/uinteger.h"
#include "memory-pool.h"

namespace ns3 {

GlobalValue gSharedMemoryPoolSize =
    GlobalValue ("SharedMemoryPoolSize", "Shared Memory Pool Size", UintegerValue (4096),
                 MakeUintegerChecker<uint32_t> ());
GlobalValue gSharedMemoryKey = GlobalValue ("SharedMemoryKey", "Shared Memory Key",
                                            UintegerValue (1234), MakeUintegerChecker<uint32_t> ());

SharedMemoryPool::SharedMemoryPool (void)
{
  UintegerValue uv;
  gSharedMemoryPoolSize.GetValue (uv);
  m_memoryPoolSize = uv.Get ();
  gSharedMemoryKey.GetValue (uv);
  m_memoryKey = uv.Get ();
  memset (m_memoryCtrlInfo, 0, sizeof (m_memoryCtrlInfo));
  memset (m_memoryLocker, 0, sizeof (m_memoryLocker));
  m_ctrlBlockSize = sizeof (SharedMemoryCtrl);
  m_configLen = sizeof (CtrlInfoBlock);

  m_shmid = shmget (m_memoryKey, m_memoryPoolSize, 0666 | IPC_CREAT);
  NS_ASSERT_MSG (m_shmid >= 0, "Cannot alloc shared memory(shmid<0)");

  m_memoryPoolPtr = (uint8_t *) shmat (m_shmid, NULL, 0);
  NS_ASSERT_MSG (m_memoryPoolPtr > 0, "Cannot alloc shared memory(ptr error)");

  shmctl (m_shmid, IPC_STAT, &m_shmds);
  m_isCreator = getpid () == m_shmds.shm_cpid;
  if (m_isCreator)
    {
      // puts("Creator");
      memset (m_memoryPoolPtr, 0, m_memoryPoolSize);
    }
  else
    {
      // puts("User");
      sleep (1);
    }

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
      // printf("Load %u\n", m_curCtrlInfo->id);
    }
  m_currentVersion = m_ctrlInfo->ctrlInfoVersion;
  CtrlInfoUnlock ();
}

SharedMemoryPool::~SharedMemoryPool (void)
{
}

void
SharedMemoryPool::FreeMemory (void)
{
  if (!m_isCreator)
    return;
  shmctl (m_shmid, IPC_STAT, &m_shmds);
  while (m_shmds.shm_nattch != 1)
    shmctl (m_shmid, IPC_STAT, &m_shmds);
  shmctl (m_shmid, IPC_RMID, 0);
}

void
SharedMemoryPool::CtrlInfoLock (void)
{
  while (!__sync_bool_compare_and_swap (&m_ctrlInfo->ctrlInfoLock, 0x0, 0xffff))
    ;
}
void
SharedMemoryPool::CtrlInfoUnlock (void)
{
  NS_ASSERT_MSG (__sync_bool_compare_and_swap (&m_ctrlInfo->ctrlInfoLock, 0xffff, 0x0),
                 "Lock status error");
}

void *
SharedMemoryPool::GetMemory (uint16_t id, uint32_t size)
{
  NS_ASSERT_MSG (id < SHM_MAX_KEY_ID, "Id out of range");
  CtrlInfoLock ();
  if (m_ctrlInfo->ctrlInfoVersion != m_currentVersion)
    {
      while (m_curCtrlInfo->ctrlInfo == FollowCtrlBlock)
        {
          --m_curCtrlInfo;
          if (m_memoryCtrlInfo[m_curCtrlInfo->id] != 0)
            {
              std::cerr << "Id " << m_curCtrlInfo->id << " has been used" << std::endl;
              NS_ABORT_MSG ("Id error");
            }
          // NS_ASSERT_MSG (m_memoryCtrlInfo[m_curCtrlInfo->id] == 0, "Id %u has been used",
          //                m_curCtrlInfo->id);
          m_memoryCtrlInfo[m_curCtrlInfo->id] = m_curCtrlInfo;
          // printf("Load %u\n", m_curCtrlInfo->id);
        }
      m_currentVersion = m_ctrlInfo->ctrlInfoVersion;
    }
  if (m_memoryCtrlInfo[id] != 0)
    {
      NS_ASSERT_MSG (size == m_memoryCtrlInfo[id]->size, "Size of memory error");
      CtrlInfoUnlock ();
      return m_memoryPoolPtr + m_memoryCtrlInfo[id]->offset;
    };
  // printf("Alloc id %u\n", id);

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
  m_memoryLocker[id] =
      (SharedMemoryLockable *) GetMemory (id, size + sizeof (SharedMemoryLockable));
  return m_memoryLocker[id]->mem;
}

void *SharedMemoryPool::AcquireMemory (uint16_t id) //Should register first
{
  SharedMemoryLockable *info = m_memoryLocker[id];
  while (
      !__sync_bool_compare_and_swap (&info->preVersion, info->version, info->version + (uint8_t) 1))
    ;
  return info->mem;
}

void SharedMemoryPool::ReleaseMemory (uint16_t id) //Should register first
{
  SharedMemoryLockable *info = m_memoryLocker[id];
  NS_ASSERT_MSG (__sync_bool_compare_and_swap (&info->version, info->preVersion - (uint8_t) 1,
                                               info->preVersion),
                 "Lock status error");
}

uint8_t
SharedMemoryPool::GetMemoryVersion (uint8_t id)
{
  return m_memoryLocker[id]->version;
}
} // namespace ns3