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

#include "ns3/boolean.h"
#include "ns3/burst-sink-helper.h"
#include "ns3/bursty-helper.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/pointer.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/system-path.h"
#include "ns3/trace-file-burst-generator.h"
#include "ns3/trace-helper.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <iomanip>

// This example was built starting from examples/wireless/wifi-simple-ht-hidden-stations.cc
//
// Network topology: `nStas` are equally spaced over a circle of radius `distance` from the AP.
// Example with 2 nodes:
//
//  n1
//  |
//  *
//
//  AP
//
//  *
//  |
//  n2
//
// The results of this script are shown in the first reference paper (see README.md).

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VrAppNStas");

std::vector<std::string>
SplitString(const std::string& str, char delimiter)
{
    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> container;

    while (getline(ss, token, delimiter))
    {
        container.push_back(token);
    }
    return container;
}

std::string
GetInputPath()
{
    std::string systemPath = SystemPath::FindSelfDirectory();
    std::vector<std::string> pathComponents = SplitString(systemPath, '/');

    std::string inputPath = "/";
    std::string dir;
    for (size_t i = 0; i < pathComponents.size(); ++i)
    {
        dir = pathComponents.at(i);
        if (dir == "")
            continue;
        inputPath += dir + "/";
        if (dir == "ns-3-dev")
            break;
    }
    return inputPath;
}

std::string
AddressToString(const Address& addr)
{
    std::stringstream addressStr;
    addressStr << InetSocketAddress::ConvertFrom(addr).GetIpv4();
    return addressStr.str();
}

void
BurstRx(Ptr<OutputStreamWrapper> traceFile,
        Ptr<const Packet> burst,
        const Address& from,
        const Address& to,
        const SeqTsSizeFragHeader& header)
{
    *traceFile->GetStream() << AddressToString(from) << "," << header.GetTs().GetNanoSeconds()
                            << "," << Simulator::Now().GetNanoSeconds() << "," << header.GetSeq()
                            << "," << header.GetSize() << "\n";
}

void
FragmentRx(Ptr<OutputStreamWrapper> traceFile,
           Ptr<const Packet> fragment,
           const Address& from,
           const Address& to,
           const SeqTsSizeFragHeader& header)
{
    *traceFile->GetStream() << AddressToString(from) << "," << header.GetTs().GetNanoSeconds()
                            << "," << Simulator::Now().GetNanoSeconds() << "," << header.GetSeq()
                            << "," << header.GetFragSeq() << "," << header.GetFrags() << ","
                            << fragment->GetSize() << "\n";
}

int
main(int argc, char* argv[])
{
    uint32_t nStas = 2;                    // the number of STAs around the AP
    double distance = 1;                   // the distance from the AP [m]
    std::string appRate = "50Mbps";        // the app target data rate
    double frameRate = 60;                 // the app frame rate [FPS]
    std::string vrAppName = "VirusPopper"; // the app name
    std::string burstGeneratorType =
        "model";                // type of burst generator {"model", "trace", "deterministic"}
    double simulationTime = 10; // simulation time in seconds

    CommandLine cmd(__FILE__);
    cmd.AddValue("nStas", "the number of STAs around the AP", nStas);
    // cmd.AddValue ("distance", "the distance from the AP [m]", distance);
    cmd.AddValue("appRate", "the app target data rate", appRate);
    cmd.AddValue("frameRate", "the app frame rate [FPS]", frameRate);
    cmd.AddValue("vrAppName", "the app name", vrAppName);
    cmd.AddValue("burstGeneratorType",
                 "type of burst generator {\"model\", \"trace\", \"deterministic\"}",
                 burstGeneratorType);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.Parse(argc, argv);

    uint32_t fragmentSize = 1472; // bytes
    uint32_t channelWidth = 160;  // MHz
    double frequency = 5;         // GHz
    bool sgi = true;              // Use short guard interval

    LogComponentEnableAll(LOG_PREFIX_ALL);
    LogComponentEnable("VrAppNStas", LOG_INFO);
    LogComponentEnable("BurstSink", LOG_INFO);

    // Disable RTS/CTS
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("999999"));

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nStas);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("VhtMcs9"),
                                 "ControlMode",
                                 StringValue("VhtMcs0"));

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    if (frequency != 5)
    {
        NS_ABORT_MSG("Frequency=" << frequency << "GHz not supported by this wifi standard.");
    }

    phy.Set("ChannelSettings",
            StringValue(std::string("{0, ") + std::to_string(channelWidth) + ", BAND_5GHZ, 0}"));

    WifiMacHelper mac;
    Ssid ssid = Ssid("vr-app-n-stas");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(ssid),
                "EnableBeaconJitter",
                BooleanValue(false));

    NetDeviceContainer apDevice;
    apDevice = wifi.Install(phy, mac, wifiApNode);

    Config::Set(
        "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported",
        BooleanValue(sgi));
    // Setting mobility model
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // AP is in the center
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));

    // STAs around the AP in a circle of fixed radius
    double dTheta = 2 * M_PI / nStas;
    for (uint32_t i = 0; i < nStas; i++)
    {
        positionAlloc->Add(
            Vector(std::cos(i * dTheta) * distance, std::sin(i * dTheta) * distance, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer StaInterface;
    StaInterface = address.Assign(staDevices);
    Ipv4InterfaceContainer ApInterface;
    ApInterface = address.Assign(apDevice);

    // Setting applications
    uint16_t port = 50000;
    BurstSinkHelper server("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer serverApp = server.Install(wifiApNode);
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(simulationTime + 1));

    BurstyHelper client("ns3::UdpSocketFactory",
                        InetSocketAddress(ApInterface.GetAddress(0), port));
    client.SetAttribute("FragmentSize", UintegerValue(fragmentSize));

    if (burstGeneratorType == "model")
    {
        NS_LOG_DEBUG("VR generator with framerate=" << frameRate << ", appRate=" << appRate
                                                    << ", vrAppName=" << vrAppName);

        client.SetBurstGenerator("ns3::VrBurstGenerator",
                                 "FrameRate",
                                 DoubleValue(frameRate),
                                 "TargetDataRate",
                                 DataRateValue(DataRate(appRate)),
                                 "VrAppName",
                                 StringValue(vrAppName));
    }
    else if (burstGeneratorType == "trace")
    {
        uint32_t dataRateMbps = uint32_t(DataRate(appRate).GetBitRate() / 1e6);
        std::string appAbbrev;
        if (vrAppName == "VirusPopper")
        {
            appAbbrev = "vp";
        }
        else if (vrAppName == "Minecraft")
        {
            appAbbrev = "mc";
        }
        else if (vrAppName == "GoogleEarthVrCities")
        {
            appAbbrev = "ge_cities";
        }
        else if (vrAppName == "GoogleEarthVrTour")
        {
            appAbbrev = "ge_tour";
        }
        else
        {
            NS_ABORT_MSG("vrAppName=" << vrAppName << " was not recognized");
        }

        std::ostringstream filenameSs;
        filenameSs << GetInputPath() << "./BurstGeneratorTraces/" << appAbbrev << "_"
                   << dataRateMbps << "mbps_" << uint32_t(frameRate) << "fps.csv";

        NS_LOG_DEBUG("Trace file generator with filename=" << filenameSs.str());

        client.SetBurstGenerator("ns3::TraceFileBurstGenerator",
                                 "TraceFile",
                                 StringValue(filenameSs.str()));
    }
    else if (burstGeneratorType == "deterministic")
    {
        double avgPeriod = 1 / frameRate;
        double avgBurstSize = DataRate(appRate).GetBitRate() / 8.0 / frameRate;

        std::ostringstream periodSs;
        periodSs << "ns3::ConstantRandomVariable[Constant=" << avgPeriod << "]";
        std::ostringstream burstSizeSs;
        burstSizeSs << "ns3::ConstantRandomVariable[Constant=" << avgBurstSize << "]";

        NS_LOG_DEBUG("Simple generator with period=" << periodSs.str()
                                                     << ", burstSize=" << burstSizeSs.str());

        client.SetBurstGenerator("ns3::SimpleBurstGenerator",
                                 "PeriodRv",
                                 StringValue(periodSs.str()),
                                 "BurstSizeRv",
                                 StringValue(burstSizeSs.str()));
    }
    else
    {
        NS_FATAL_ERROR("burstGeneratorType=" << burstGeneratorType << " not recognized");
    }

    // Saturated UDP traffic from stations to AP
    ApplicationContainer clientApps = client.Install(wifiStaNodes);
    Ptr<UniformRandomVariable> x =
        CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                          DoubleValue(0),
                                                          "Max",
                                                          DoubleValue(1));
    Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable>();
    for (uint32_t i = 0; i < nStas; i++)
    {
        Time startTime = Seconds(x->GetValue());
        NS_LOG_UNCOND("STA" << i << " will start at " << startTime.As(Time::S));
        Ptr<BurstyApplication> app = DynamicCast<BurstyApplication>(clientApps.Get(i));
        app->SetStartTime(startTime);

        // decouple trace file burst generators
        PointerValue val;
        app->GetAttribute("BurstGenerator", val);
        Ptr<TraceFileBurstGenerator> tfbg = DynamicCast<TraceFileBurstGenerator>(val.GetObject());
        if (tfbg)
        {
            NS_ABORT_MSG_IF(tfbg->GetTraceDuration() < simulationTime,
                            "Trace too short for this simulation");

            double traceStartTime = y->GetValue(0, tfbg->GetTraceDuration() - simulationTime);
            NS_LOG_UNCOND("STA" << i << " will start its trace from " << traceStartTime);
            tfbg->SetAttribute("StartTime", DoubleValue(traceStartTime));
        }
    }
    clientApps.Stop(Seconds(simulationTime + 1));

    // Setup traces
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> burstTrace = ascii.CreateFileStream("burstTrace.csv");
    *burstTrace->GetStream() << "SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,BurstSize" << std::endl;
    Ptr<OutputStreamWrapper> fragmentTrace = ascii.CreateFileStream("fragmentTrace.csv");
    *fragmentTrace->GetStream()
        << "SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,FragSeq,TotFrags,FragSize" << std::endl;

    DynamicCast<BurstSink>(serverApp.Get(0))
        ->TraceConnectWithoutContext("BurstRx", MakeBoundCallback(&BurstRx, burstTrace));
    DynamicCast<BurstSink>(serverApp.Get(0))
        ->TraceConnectWithoutContext("FragmentRx", MakeBoundCallback(&FragmentRx, fragmentTrace));

    // Start simulation
    Simulator::Stop(Seconds(simulationTime + 1));
    Simulator::Run();

    // burst info
    Ptr<OutputStreamWrapper> txBurstsBySta = ascii.CreateFileStream("txBurstsBySta.csv");
    Ptr<OutputStreamWrapper> rxBursts = ascii.CreateFileStream("rxBursts.csv");

    uint64_t totBurstSent = 0;
    for (uint32_t i = 0; i < nStas; i++)
    {
        uint64_t burstsSent = DynamicCast<BurstyApplication>(clientApps.Get(i))->GetTotalTxBursts();
        totBurstSent += burstsSent;

        std::cout << "burstsSent(" << i << ")=" << burstsSent << ", ";
        *txBurstsBySta->GetStream() << burstsSent << std::endl;
    }
    uint64_t burstsReceived = DynamicCast<BurstSink>(serverApp.Get(0))->GetTotalRxBursts();

    std::cout << "burstsReceived=" << burstsReceived << " ("
              << double(burstsReceived) / totBurstSent * 100 << "%)" << std::endl;
    *rxBursts->GetStream() << burstsReceived << std::endl;

    // fragment info
    Ptr<OutputStreamWrapper> txFragmentsBySta = ascii.CreateFileStream("txFragmentsBySta.csv");
    Ptr<OutputStreamWrapper> rxFragments = ascii.CreateFileStream("rxFragments.csv");

    uint64_t totFragmentSent = 0;
    for (uint32_t i = 0; i < nStas; i++)
    {
        uint64_t fragmentsSent =
            DynamicCast<BurstyApplication>(clientApps.Get(i))->GetTotalTxFragments();
        totFragmentSent += fragmentsSent;

        std::cout << "fragmentsSent(" << i << ")=" << fragmentsSent << ", ";
        *txFragmentsBySta->GetStream() << fragmentsSent << std::endl;
    }
    uint64_t fragmentsReceived = DynamicCast<BurstSink>(serverApp.Get(0))->GetTotalRxFragments();

    std::cout << "fragmentsReceived=" << fragmentsReceived << " ("
              << double(fragmentsReceived) / totFragmentSent * 100 << "%)" << std::endl;
    *rxFragments->GetStream() << fragmentsReceived << std::endl;

    Simulator::Destroy();

    return 0;
}
