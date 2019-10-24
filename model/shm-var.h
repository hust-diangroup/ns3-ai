/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#pragma once
#include "memory-pool.h"
namespace ns3 {

#ifndef READABLE
#define READABLE 0xff
#endif
#ifndef SETABLE
#define SETABLE 0x0
#endif
/*
适用于小型数据
*/
template <typename T>
class ShmVar
{
public:
  struct Interface
  {
    uint8_t tagRd;
    uint8_t tagWt;
    T data;
  } __attribute__ ((__packed__));
  Interface *m_data;

public:
  ShmVar () = delete;
  ShmVar (int id);
  ~ShmVar ();
  T Get ();
  void Set (T data);
};

template <typename T>
ShmVar<T>::ShmVar (int id)
{
  m_data = (Interface *) SharedMemoryPool::Get()->GetMemory (id, sizeof (Interface));
  NS_ASSERT_MSG (m_data > 0, "memory pool full");
}

template <typename T>
ShmVar<T>::~ShmVar ()
{
}

template <typename T>
T
ShmVar<T>::Get ()
{
  while (m_data->tagRd != READABLE)
    ;
  T ret = m_data->data;
  NS_ASSERT_MSG (__sync_bool_compare_and_swap (&m_data->tagRd, READABLE, SETABLE), "Tag status error");
  return ret;
}
template <typename T>
void
ShmVar<T>::Set (T data)
{
  while (m_data->tagWt != SETABLE)
    ;
  m_data->data = data;
  NS_ASSERT_MSG (__sync_bool_compare_and_swap (&m_data->tagWt, SETABLE, READABLE), "Tag status error");
}

} // namespace ns3