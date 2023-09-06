/*
 * Copyright (c) 2022
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
 */

#ifndef NS3_MULTI_BSS_H
#define NS3_MULTI_BSS_H

#include <array>
#include <cstdint>

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
