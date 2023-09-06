/*
 * Copyright (c) 2020-2023 Huazhong University of Science and Technology
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

#include "apb.h"

#include <ns3/ai-module.h>

#include <chrono>
#include <iostream>
#include <random>

#define NUM_ENV 10000

using namespace ns3;

int
main()
{
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(false);
    interface->SetHandleFinish(true);
    Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>* msgInterface =
        interface->GetInterface<EnvStruct, ActStruct>();

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    for (int i = 0; i < NUM_ENV; ++i)
    {
        msgInterface->CppSendBegin();
        std::cout << "set: ";
        uint32_t temp_a = distrib(gen);
        uint32_t temp_b = distrib(gen);
        std::cout << temp_a << "," << temp_b << ";";
        msgInterface->GetCpp2PyStruct()->env_a = temp_a;
        msgInterface->GetCpp2PyStruct()->env_b = temp_b;
        std::cout << "\n";
        msgInterface->CppSendEnd();

        msgInterface->CppRecvBegin();
        std::cout << "get: ";
        std::cout << msgInterface->GetPy2CppStruct()->act_c;
        std::cout << "\n";
        msgInterface->CppRecvEnd();
    }
}
