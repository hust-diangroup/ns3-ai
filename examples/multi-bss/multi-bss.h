//
// Created by 申牧原 on 2023/8/16.
//

#ifndef NS3_MULTI_BSS_H
#define NS3_MULTI_BSS_H

#include <cstdint>
#include <array>

struct Env
{
    uint32_t txNode;
    std::array<double, 5> rxPower;
    uint32_t mcs;
    double holDelay;
    double throughput;
};

struct Act
{
    double newCcaSensitivity;
};

#endif // NS3_MULTI_BSS_H
