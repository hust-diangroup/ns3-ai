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
 *
 */

#include "ns3/applications-module.h"
#include "ns3/burst-sink-helper.h"
#include "ns3/bursty-helper.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/seq-ts-size-frag-header.h"
#include "ns3/trace-file-burst-generator.h"

#include <iomanip>

/**
 * Example of BurstyApplication using a TraceFileBurstGenerator.
 * Some example traces are provided, obtained from a real VR source.
 * Please read the reference paper for further information (see README.md).
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BurstyApplicationExample");

std::string
AddressToString(const Address& addr)
{
    std::stringstream addressStr;
    addressStr << InetSocketAddress::ConvertFrom(addr).GetIpv4() << ":"
               << InetSocketAddress::ConvertFrom(addr).GetPort();
    return addressStr.str();
}

void
BurstRx(Ptr<const Packet> burst,
        const Address& from,
        const Address& to,
        const SeqTsSizeFragHeader& header)
{
    NS_LOG_INFO("Received burst seq=" << header.GetSeq() << " of " << header.GetSize()
                                      << " bytes transmitted at " << std::setprecision(9)
                                      << header.GetTs().As(Time::S));
}

int
main(int argc, char* argv[])
{
    std::string traceFolder = "./BurstGeneratorTraces/"; // example traces can be found here
    std::string traceFile = "ge_cities_20mbps_30fps.csv";
    double startTime = 0;
    double simTime = 20;

    CommandLine cmd(__FILE__);
    cmd.AddValue("traceFolder", "The folder containing the trace.", traceFolder);
    cmd.AddValue("traceFile", "The trace file name.", traceFile);
    cmd.AddValue("startTime", "The start time offset of the trace [s].", startTime);
    cmd.AddValue("simTime", "Length of simulation [s].", simTime);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnable("BurstyApplicationExample", LOG_INFO);

    // Setup two nodes
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    uint16_t portNumber = 50000;

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    Ipv4Address serverAddress = interfaces.GetAddress(0);
    Ipv4Address sinkAddress = Ipv4Address::GetAny(); // 0.0.0.0

    // Create bursty application helper
    BurstyHelper burstyHelper("ns3::UdpSocketFactory",
                              InetSocketAddress(serverAddress, portNumber));
    burstyHelper.SetAttribute("FragmentSize", UintegerValue(1200));
    burstyHelper.SetBurstGenerator("ns3::TraceFileBurstGenerator",
                                   "TraceFile",
                                   StringValue(traceFolder + traceFile),
                                   "StartTime",
                                   DoubleValue(startTime));

    // Install bursty application
    ApplicationContainer serverApps = burstyHelper.Install(nodes.Get(1));
    Ptr<BurstyApplication> burstyApp = serverApps.Get(0)->GetObject<BurstyApplication>();

    // Extract TraceFileBurstGenerator and check if able to fill the entire simulation
    PointerValue val;
    burstyApp->GetAttribute("BurstGenerator", val);
    Ptr<TraceFileBurstGenerator> tfbg = DynamicCast<TraceFileBurstGenerator>(val.GetObject());
    NS_ASSERT_MSG(tfbg, "The bursty application should be a TraceFileBurstGenerator");

    if (startTime + simTime > tfbg->GetTraceDuration())
    {
        NS_LOG_WARN("The trace file will end before the simulation ends. Please choose a different "
                    << "start time, a longer trace, or reduce the simulation duration.");
    }

    // Create burst sink helper
    BurstSinkHelper burstSinkHelper("ns3::UdpSocketFactory",
                                    InetSocketAddress(sinkAddress, portNumber));

    // Install HTTP client
    ApplicationContainer clientApps = burstSinkHelper.Install(nodes.Get(0));
    Ptr<BurstSink> burstSink = clientApps.Get(0)->GetObject<BurstSink>();

    // Example of connecting to the trace sources
    burstSink->TraceConnectWithoutContext("BurstRx", MakeCallback(&BurstRx));

    // TraceFileBurstGenerator stops automatically, but we add an early stop to make the example
    // quicker
    serverApps.Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    // Stats
    std::cout << "Total RX bursts: " << burstyApp->GetTotalTxBursts() << "/"
              << burstSink->GetTotalRxBursts() << std::endl;
    std::cout << "Total RX fragments: " << burstyApp->GetTotalTxFragments() << "/"
              << burstSink->GetTotalRxFragments() << std::endl;
    std::cout << "Total RX bytes: " << burstyApp->GetTotalTxBytes() << "/"
              << burstSink->GetTotalRxBytes() << std::endl;

    return 0;
}
