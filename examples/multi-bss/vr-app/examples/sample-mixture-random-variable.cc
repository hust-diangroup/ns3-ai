/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 SIGNET Lab, Department of Information Engineering,
 * University of Padova
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
 */
#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/my-random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

#include <iostream>
#include <vector>

/**
 * \file
 * \ingroup core-examples
 * \ingroup randomvariable
 * Example program illustrating use of ns3::MixtureRandomVariable
 */

using namespace ns3;

int
main(int argc, char* argv[])
{
    uint32_t nSamples = 1000000;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nSamples", "Number of samples", nSamples);
    cmd.Parse(argc, argv);

    Ptr<MixtureRandomVariable> x = CreateObject<MixtureRandomVariable>();

    // setup weights cdf
    std::vector<double> w{0.7, 1.0}; // p1 = 0.7, p2 = 0.3
    // setup random variables
    std::vector<Ptr<RandomVariableStream>> rvs;
    rvs.push_back(CreateObjectWithAttributes<NormalRandomVariable>("Mean",
                                                                   DoubleValue(5),
                                                                   "Variance",
                                                                   DoubleValue(1)));
    rvs.push_back(CreateObjectWithAttributes<NormalRandomVariable>("Mean",
                                                                   DoubleValue(10),
                                                                   "Variance",
                                                                   DoubleValue(4)));

    x->SetRvs(w, rvs);

    for (uint32_t i = 0; i < nSamples; i++)
    {
        std::cout << x->GetValue() << std::endl;
    }

    return 0;
}
