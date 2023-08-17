#include <iostream>
#include <random>
#include <chrono>
#include <ns3/ai-module.h>

#include "apb.h"

#define NUM_ENV 10000

using namespace ns3;

int main() {
    Ns3AiMsgInterface<EnvStruct, ActStruct> cpp_side(false, false, true);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    for (int i = 0; i < NUM_ENV; ++i) {
        cpp_side.cpp_send_begin();
        std::cout << "set: ";
        uint32_t temp_a = distrib(gen);
        uint32_t temp_b = distrib(gen);
        std::cout << temp_a << "," << temp_b << ";";
        cpp_side.m_single_cpp2py_msg->env_a = temp_a;
        cpp_side.m_single_cpp2py_msg->env_b = temp_b;
        std::cout << "\n";
        cpp_side.cpp_send_end();

        cpp_side.cpp_recv_begin();
        std::cout << "get: ";
        std::cout << cpp_side.m_single_py2cpp_msg->act_c;
        std::cout << "\n";
        cpp_side.cpp_recv_end();
    }

}

