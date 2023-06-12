//
// Created by 申牧原 on 2023/6/12.
//
#include "ai-thompson-sampling-wifi-manager.h"
#include <ns3/ns3-ai-module.h>

using namespace ns3;

int main() {
    NS3AIRL<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct> py_side(4096, true);
    std::vector<AiThompsonSamplingEnvStruct> env;
    std::vector<AiThompsonSamplingActStruct> act;

    act.clear();
    py_side.get_env(env);
    assert(env.size() == 1);
    std::cout << "env[0] at addr " << reinterpret_cast<void *>(&env.at(0)) << std::endl;
    int n_stats = env.at(0).data.stats.size();
    for (int i = 0; i < n_stats; ++i) {
        std::cout << "nss at index " << i
                  << " is " << (uint32_t) (env.at(0).data.stats.at(i).nss)
                  << " addr " << reinterpret_cast<void *>(&env.at(0).data.stats.at(i).nss) << std::endl;
    }

}
