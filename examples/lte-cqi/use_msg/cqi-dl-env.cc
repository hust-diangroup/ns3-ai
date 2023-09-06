/*
 * Copyright (c) 2019-2023 Huazhong University of Science and Technology
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
 * Authors: Pengyu Liu <eic_lpy@hust.edu.cn>
 *          Hao Yin <haoyin@uw.edu>
 *          Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#include "cqi-dl-env.h"

/**
 * \brief Link the shared memory with the id and set the operation lock
 *
 * \param[in] id  shared memory id, should be the same in python and ns-3
 */
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("cqi-dl-env");

NS_OBJECT_ENSURE_REGISTERED(CQIDL);

CQIDL::CQIDL()
{
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(false);
    interface->SetHandleFinish(true);
}

CQIDL::~CQIDL()
{
}

TypeId
CQIDL::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CQIDL").SetParent<Object>().SetGroupName("Ns3Ai").AddConstructor<CQIDL>();
    return tid;
}

/**
 * \brief Set the value of wbcqi.
 *
 * \param[in] cqi  the value of wbcqi to be set
 */
void
CQIDL::SetWbCQI(uint8_t cqi)
{
    Ns3AiMsgInterfaceImpl<CqiFeature, CqiPredicted>* msgInterface =
        Ns3AiMsgInterface::Get()->GetInterface<CqiFeature, CqiPredicted>();
    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->wbCqi = cqi;
    msgInterface->CppSendEnd();
}

/**
 * \brief Get the predictive value of wbcqi.
 *
 * \returns the predictive value of wbcqi
 */
uint8_t
CQIDL::GetWbCQI()
{
    Ns3AiMsgInterfaceImpl<CqiFeature, CqiPredicted>* msgInterface =
        Ns3AiMsgInterface::Get()->GetInterface<CqiFeature, CqiPredicted>();
    msgInterface->CppRecvBegin();
    uint8_t ret = msgInterface->GetPy2CppStruct()->new_wbCqi;
    msgInterface->CppRecvEnd();
    return ret;
}

} // namespace ns3
