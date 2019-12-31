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
#include "memory-pool.h"
namespace ns3 {

#ifndef READABLE
#define READABLE 0xff
#endif
#ifndef SETABLE
#define SETABLE 0x0
#endif

template <typename T>
class TrainVar
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
  TrainVar () = delete;
  TrainVar (int id);
  ~TrainVar ();
  T Get ();
  void Set (T data);
};

template <typename T>
TrainVar<T>::TrainVar (int id)
{
  m_data = (Interface *) SharedMemoryPool::Get ()->GetMemory (id, sizeof (Interface));
  NS_ASSERT_MSG (m_data > 0, "memory pool full");
}

template <typename T>
TrainVar<T>::~TrainVar ()
{
}

template <typename T>
T
TrainVar<T>::Get ()
{
  while (m_data->tagRd != READABLE)
    ;
  T ret = m_data->data;
  NS_ASSERT_MSG (__sync_bool_compare_and_swap (&m_data->tagRd, READABLE, SETABLE),
                 "Tag status error");
  return ret;
}
template <typename T>
void
TrainVar<T>::Set (T data)
{
  while (m_data->tagWt != SETABLE)
    ;
  m_data->data = data;
  NS_ASSERT_MSG (__sync_bool_compare_and_swap (&m_data->tagWt, SETABLE, READABLE),
                 "Tag status error");
}

} // namespace ns3