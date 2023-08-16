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
#include "ns3/bursty-app-stats-calculator.h"
#include "ns3/bursty-helper.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/seq-ts-size-frag-header.h"

/**
 * An example on how to use the BurstyAppStatsCalculator.
 * Example based on bursty-application-example.
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
BurstTx(uint32_t nodeId,
        Ptr<BurstyAppStatsCalculator> statsCalculator,
        Ptr<const Packet> burst,
        const Address& from,
        const Address& to,
        const SeqTsSizeFragHeader& header)
{
    NS_LOG_INFO("Sent burst seq=" << header.GetSeq() << " of header.GetSize ()=" << header.GetSize()
                                  << " (burst->GetSize ()=" << burst->GetSize() << ") bytes from "
                                  << AddressToString(from) << " to " << AddressToString(to)
                                  << " at " << header.GetTs().As(Time::S));
    statsCalculator->TxBurst(nodeId, burst, from, to, header);
}

void
BurstRx(uint32_t nodeId,
        Ptr<BurstyAppStatsCalculator> statsCalculator,
        Ptr<const Packet> burst,
        const Address& from,
        const Address& to,
        const SeqTsSizeFragHeader& header)
{
    NS_LOG_INFO("Received burst seq="
                << header.GetSeq() << " of header.GetSize ()=" << header.GetSize()
                << " (burst->GetSize ()=" << burst->GetSize() << ") bytes from "
                << AddressToString(from) << " to " << AddressToString(to) << " at "
                << header.GetTs().As(Time::S));

    statsCalculator->RxBurst(nodeId, burst, from, to, header);
}

int
main(int argc, char* argv[])
{
    double simTimeSec = 10;
    CommandLine cmd(__FILE__);
    cmd.AddValue("SimulationTime", "Length of simulation in seconds.", simTimeSec);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnable("BurstyApplicationExample", LOG_INFO);

    Config::SetDefault("ns3::BurstyAppStatsCalculator::EpochDuration", TimeValue(Seconds(0.1)));
    Config::SetDefault("ns3::BurstyAppStatsCalculator::WriteToFile", BooleanValue(true));

    // Setup two nodes
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
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
    burstyHelper.SetBurstGenerator("ns3::SimpleBurstGenerator",
                                   "PeriodRv",
                                   StringValue("ns3::ConstantRandomVariable[Constant=100e-3]"),
                                   "BurstSizeRv",
                                   StringValue("ns3::ConstantRandomVariable[Constant=10e3]"));

    // Install bursty application
    ApplicationContainer serverApps = burstyHelper.Install(nodes.Get(1));
    Ptr<BurstyApplication> burstyApp = serverApps.Get(0)->GetObject<BurstyApplication>();
    Ptr<BurstyAppStatsCalculator> statsCalculator = CreateObject<BurstyAppStatsCalculator>();

    // Example of connecting to the trace sources
    burstyApp->TraceConnectWithoutContext(
        "BurstTx",
        MakeBoundCallback(&BurstTx, nodes.Get(1)->GetId(), statsCalculator));

    // Create burst sink helper
    BurstSinkHelper burstSinkHelper("ns3::UdpSocketFactory",
                                    InetSocketAddress(sinkAddress, portNumber));

    // Install burst sink
    ApplicationContainer clientApps = burstSinkHelper.Install(nodes.Get(0));
    Ptr<BurstSink> burstSink = clientApps.Get(0)->GetObject<BurstSink>();

    // Example of connecting to the trace sources
    burstSink->TraceConnectWithoutContext(
        "BurstRx",
        MakeBoundCallback(&BurstRx, nodes.Get(0)->GetId(), statsCalculator));

    // Stop bursty app after simTimeSec
    serverApps.Stop(Seconds(simTimeSec));
    Simulator::Stop(Seconds(simTimeSec));
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
