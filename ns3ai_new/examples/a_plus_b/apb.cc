#include <iostream>
#include <random>
#include <chrono>

#include <ns3/ns3-ai-new-rl.h>
#include "apb.h"

using namespace ns3;

int
main()
{
    NS3AIRL<EnvStruct, ActStruct> cpp_side(4096);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    for (int i = 0; i < 10; ++i)
    {
        cpp_side.m_local_env.clear();
        std::cout << "CPP env to set: ";
        for (int j = 0; j < 3; ++j)
        {
            uint32_t temp_a = distrib(gen);
            uint32_t temp_b = distrib(gen);
            std::cout << temp_a << "," << temp_b << ";";
            cpp_side.m_local_env.push_back({temp_a, temp_b});
        }
        std::cout << "\n";
        cpp_side.set_env();

        cpp_side.get_act();
        assert(cpp_side.m_local_env.size() == cpp_side.m_local_act.size());
        std::cout << "Get act: ";
        for (ActStruct j : cpp_side.m_local_act)
        {
            std::cout << j.act_c << ";";
        }
        std::cout << "\n";

        // processing time
        //        sleep(1);
    }
}

