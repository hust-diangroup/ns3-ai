#include <iostream>
#include <random>
#include <chrono>
#include <ns3/ns3-ai-module.h>

#include "apb.h"

#define NUM_ENV 10000
#define APB_SIZE 3

using namespace ns3;

int main() {
    Ns3AiMsgInterface<EnvStruct, ActStruct> cpp_side(4096, true);

    // Should run after Python
    assert(cpp_side.m_cpp2py_msg->size() == APB_SIZE);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    for (int i = 0; i < NUM_ENV; ++i) {
        cpp_side.cpp_send_begin();
        std::cout << "set: ";
        for (int j = 0; j < APB_SIZE; ++j) {
            uint32_t temp_a = distrib(gen);
            uint32_t temp_b = distrib(gen);
            std::cout << temp_a << "," << temp_b << ";";
            cpp_side.m_cpp2py_msg->at(j).env_a = temp_a;
            cpp_side.m_cpp2py_msg->at(j).env_b = temp_b;
        }
        std::cout << "\n";
        cpp_side.cpp_send_end();

        cpp_side.cpp_recv_begin();
        std::cout << "get: ";
        for (ActStruct j: *cpp_side.m_py2cpp_msg) {
            std::cout << j.act_c << ";";
        }
        std::cout << "\n";
        cpp_side.cpp_recv_end();
    }

}

