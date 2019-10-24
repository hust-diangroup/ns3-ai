/* -*- Mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
  uint8_t version;
  uint8_t preVersion;
  uint8_t mem[0];
} Packed;

void Init (uint32_t key, uint32_t poolSize);
void FreeMemory (void);
void *GetMemory (uint16_t id, uint32_t size);
void *RegisterMemory (uint16_t id, uint32_t size);
void *AcquireMemory (uint16_t id);
void ReleaseMemory (uint16_t id);
uint8_t GetMemoryVersion (uint8_t id);

class SharedMemoryPool : public Singleton<SharedMemoryPool>
{
public:
  uint8_t *m_memoryPoolPtr;
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
  void *RegisterMemory (uint16_t id, uint32_t size);
  void *AcquireMemory (uint16_t id);
  void ReleaseMemory (uint16_t id);
  uint8_t GetMemoryVersion (uint8_t id);
};

} // namespace ns3