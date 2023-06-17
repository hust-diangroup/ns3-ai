#include <iostream>
#include <random>
#include <chrono>

#include <ns3/ns3-ai-new-rl.h>
#include "apb.h"

#define NUM_ENV 10000

using namespace ns3;

int main() {
    NS3AIRL<EnvStruct, ActStruct> cpp_side(4096);
    assert(cpp_side.m_env->empty());
    cpp_side.m_env->resize(3, {0, 0});

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    for (int i = 0; i < NUM_ENV; ++i) {
        cpp_side.set_env_begin();
        std::cout << "CPP env to set: ";
        for (int j = 0; j < 3; ++j) {
            uint32_t temp_a = distrib(gen);
            uint32_t temp_b = distrib(gen);
            std::cout << temp_a << "," << temp_b << ";";
            cpp_side.m_env->at(j).env_a = temp_a;
            cpp_side.m_env->at(j).env_b = temp_b;
        }
        std::cout << "\n";
        cpp_side.set_env_end();

        cpp_side.get_act_begin();
        std::cout << "Get act: ";
        for (ActStruct j: *cpp_side.m_act) {
            std::cout << j.act_c << ";";
        }
        std::cout << "\n";
        cpp_side.get_act_end();

        // processing time
        //        sleep(1);
    }

}

