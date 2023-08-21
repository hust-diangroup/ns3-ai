#include <iostream>
#include <random>
#include <chrono>
#include <ns3/ai-module.h>

#include "apb.h"

#define NUM_ENV 10000

using namespace ns3;

int main() {
    Ns3AiMsgInterface::Get()->SetIsMemoryCreator(false);
    Ns3AiMsgInterface::Get()->SetUseVector(false);
    Ns3AiMsgInterface::Get()->SetHandleFinish(true);
    Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct> *msgInterface = 
        Ns3AiMsgInterface::Get()->GetInterface<EnvStruct, ActStruct>();

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    for (int i = 0; i < NUM_ENV; ++i) {
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

