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
 *         Hao Yin <haoyin@uw.edu>
 */

#include "cqi-dl-env.h"

/**
 * \brief Link the shared memory with the id and set the operation lock
 * 
 * \param[in] id  shared memory id, should be the same in python and ns-3
 */
namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("cqi-dl-env");

NS_OBJECT_ENSURE_REGISTERED(CQIDL);

Ns3AiMsgInterface<CqiFeature, CqiPredicted> m_msgInterface =
    Ns3AiMsgInterface<CqiFeature, CqiPredicted>(false, false, true);

CQIDL::CQIDL()
{
//  SetCond (2, 0);
}

CQIDL::~CQIDL()
{

}

TypeId
CQIDL::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::CQIDL")
                          .SetParent<Object> ()
                          .SetGroupName ("Ns3Ai")
                          .AddConstructor<CQIDL> ()
      ;
  return tid;
}

/**
 * \brief Set the value of wbcqi.
 * 
 * \param[in] cqi  the value of wbcqi to be set
 */
void
CQIDL::SetWbCQI (uint8_t cqi)
{
    m_msgInterface.cpp_send_begin();
    m_msgInterface.m_single_cpp2py_msg->wbCqi = cqi;
    m_msgInterface.cpp_send_end();
}

/**
 * \brief Get the predictive value of wbcqi.
 * 
 * \returns the predictive value of wbcqi
 */
uint8_t
CQIDL::GetWbCQI ()
{
  m_msgInterface.cpp_recv_begin();
  uint8_t ret = m_msgInterface.m_single_py2cpp_msg->new_wbCqi;
  m_msgInterface.cpp_recv_end();
  return ret;
}

} // namespace ns3
