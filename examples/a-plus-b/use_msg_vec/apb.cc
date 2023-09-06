#include "apb.h"

#include <ns3/ai-module.h>

#include <chrono>
#include <iostream>
#include <random>

#define NUM_ENV 10000
#define APB_SIZE 3

using namespace ns3;

int
main()
{
    Ns3AiMsgInterface::Get()->SetIsMemoryCreator(false);
    Ns3AiMsgInterface::Get()->SetUseVector(true);
    Ns3AiMsgInterface::Get()->SetHandleFinish(true);
    Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()->GetInterface<EnvStruct, ActStruct>();

    // Should run after Python
    assert(msgInterface->GetCpp2PyVector()->size() == APB_SIZE);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    for (int i = 0; i < NUM_ENV; ++i)
    {
        msgInterface->CppSendBegin();
        std::cout << "set: ";
        for (int j = 0; j < APB_SIZE; ++j)
        {
            uint32_t temp_a = distrib(gen);
            uint32_t temp_b = distrib(gen);
            std::cout << temp_a << "," << temp_b << ";";
            msgInterface->GetCpp2PyVector()->at(j).env_a = temp_a;
            msgInterface->GetCpp2PyVector()->at(j).env_b = temp_b;
        }
        std::cout << "\n";
        msgInterface->CppSendEnd();

        msgInterface->CppRecvBegin();
        std::cout << "get: ";
        for (ActStruct j : *msgInterface->GetPy2CppVector())
        {
            std::cout << j.act_c << ";";
        }
        std::cout << "\n";
        msgInterface->CppRecvEnd();
    }
}
