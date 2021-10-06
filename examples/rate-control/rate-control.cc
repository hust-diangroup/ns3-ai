/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Based on 'examples/tutorials/third.cc'
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include <fstream>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RateControl");

GlobalValue gThompsonSamplingStream (
  "TSStream", 
  "Stream Value of Thompson Sampling",
  IntegerValue (100),
  MakeIntegerChecker<int64_t> ()
);

// This structure stores global variables, which are needed to calculate throughput and delay every second
struct DataForThpt
{
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;
  uint32_t totalRxPackets; //Total number of received packets in all flows
  uint64_t totalRxBytes; // Total bytes received in all flows
  double totalDelaySum; // Total delay sum in all flows
  // average delay (ms)
  double
  averageDelay ()
  {
    return totalRxPackets ? totalDelaySum / totalRxPackets / 1000000 : 0;
  }
} data; //data is a structure variable which will store all these global variables.

double duration = 1.0; // Duration of simulation (s)
double statInterval = 0.1; // Time interval of calling function Throughput

// This function is being called every 'statInterval' seconds, It measures delay and throughput in every 'statInterval' time window.
// It calculates overall throughput in that window of all flows in the network.
static void
Throughput ()
{
  data.monitor->CheckForLostPackets ();
  // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (data.flowmon.GetClassifier ());
  const FlowMonitor::FlowStatsContainer stats = data.monitor->GetFlowStats ();

  uint64_t totalRxBytes = 0;
  uint32_t totalRxPackets = 0;
  double totalDelaySum = 0;

  // Iterating through every flow
  for (FlowMonitor::FlowStatsContainerCI iter = stats.begin (); iter != stats.end (); iter++)
    {
      totalRxBytes += iter->second.rxBytes;
      totalDelaySum += iter->second.delaySum.GetDouble ();
      totalRxPackets += iter->second.rxPackets;
    }
  uint64_t rxBytesDiff = totalRxBytes - data.totalRxBytes;
  uint32_t rxPacketsDiff = totalRxPackets - data.totalRxPackets;
  double delayDiff = totalDelaySum - data.totalDelaySum;

  data.totalRxBytes = totalRxBytes;
  data.totalRxPackets = totalRxPackets;
  data.totalDelaySum = totalDelaySum;

  double delay = 0.0; // ms
  if (rxPacketsDiff != 0 && delayDiff != 0)
    {
      delay = delayDiff / rxPacketsDiff / 1000000;
    }
  double tpt = 8.0 * rxBytesDiff / statInterval / (1024 * 1024); // Mbps

  std::cout << "Delay: " << delay << "ms, Throughput: " << tpt << "Mbps" << std::endl;
  Simulator::Schedule (Seconds (statInterval), &Throughput);
}

int
setWifiStandard (WifiHelper &wifi, const std::string standard)
{
  if (standard == "11a")
    {
      wifi.SetStandard (WIFI_STANDARD_80211a);
    }
  else if (standard == "11n_2_4GHZ")
    {
      wifi.SetStandard (WIFI_STANDARD_80211n_2_4GHZ);
    }
  else if (standard == "11n_5GHZ")
    {
      wifi.SetStandard (WIFI_STANDARD_80211n_5GHZ);
    }
  else if (standard == "11ac")
    {
      wifi.SetStandard (WIFI_STANDARD_80211ac);
    }
  else if (standard == "11ax_2_4GHZ")
    {
      wifi.SetStandard (WIFI_STANDARD_80211ax_2_4GHZ);
    }
  else if (standard == "11ax_5GHZ")
    {
      wifi.SetStandard (WIFI_STANDARD_80211ax_5GHZ);
    }
  else
    {
      std::cout << "Unknown OFDM standard" << std::endl;
      return 1;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  // LogComponentEnable ("AiThompsonSamplingWifiManager", LOG_LEVEL_ALL);
  
  bool tracing = false;
  bool verbose = true;
  uint32_t nCsma = 3; // Number of CSMA(LAN) nodes
  uint32_t nWifi = 3; // Number of STA(Stations)
  uint32_t maxBytes = 0;

  std::string errorModelType = "ns3::NistErrorRateModel"; // Error Model
  std::string raaAlgo = "MinstrelHt"; // RAA algorithm (WifiManager Class)
  std::string standard = "11ac";

  //Variables to set rates of various channels in topology, Refer base topology structure.
  uint32_t csmaRate = 150;
  uint32_t csmaDelay = 9000;
  uint32_t p2pRate = 50;
  uint32_t p2pDelay = 10;

  //Command-Line argument to make it interactive.
  CommandLine cmd (__FILE__);
  cmd.AddValue ("duration", "Duration of simulation (s)", duration);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("raa", "MinstrelHt / ConstantRate / Ideal", raaAlgo);
  cmd.AddValue ("maxBytes", "Max number of Bytes to be sent", maxBytes);
  cmd.AddValue ("p2pRate", "Mbps", p2pRate);
  cmd.AddValue ("p2pDelay", "MilliSeconds", p2pDelay);
  cmd.AddValue ("csmaDelay", "NanoSeconds", csmaDelay);
  cmd.AddValue ("csmaRate", "Mbps", csmaRate);
  cmd.AddValue ("standard", "WiFi standard", standard);
  cmd.Parse (argc, argv);
  std::cout << "nWifi: " << nWifi << ", RAA Algorithm: " << raaAlgo << ", duration: " << duration
            << std::endl;

  raaAlgo = "ns3::" + raaAlgo + "WifiManager";

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box"
                << std::endl;
      return 1;
    }

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (std::to_string (p2pRate) + "Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (std::to_string (p2pDelay) + "ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (std::to_string (csmaRate) + "Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (csmaDelay)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();

  // Delay model
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  // Loss model
  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue (0.3),
                              "ReferenceLoss", DoubleValue (4.0));

  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  // Error Model
  phy.SetErrorRateModel (errorModelType);

  WifiHelper wifi;

  // Setting Wifi Standard]
  setWifiStandard (wifi, standard); // wifi.SetStandard (WIFI_STANDARD_80211ax_5GHZ);

  // Setting Raa Algorithm, refer to 'src/wifi/model/rate-control'
  wifi.SetRemoteStationManager (raaAlgo);

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  if(raaAlgo == "ns3::ThompsonSamplingWifiManager")
    {
      IntegerValue ival;
      gThompsonSamplingStream.GetValue (ival);
      NS_LOG_UNCOND("ThompsonSamplingWifiManager stream " << ival.Get ());
      wifi.AssignStreams(apDevices, ival.Get ());
      wifi.AssignStreams(staDevices, ival.Get ());
    }

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (0.0), "MinY",
                                 DoubleValue (0.0), "DeltaX", DoubleValue (5.0), "DeltaY",
                                 DoubleValue (10.0), "GridWidth", UintegerValue (3), "LayoutType",
                                 StringValue ("RowFirst"));

  // Bounds for the Rectangle Grid
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Speed",
                             StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"), "Bounds",
                             RectangleValue (Rectangle (-100, 100, -100, 100)));
  mobility.Install (wifiStaNodes);

  // Setting Mobility model
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  NS_LOG_INFO ("Create Applications.");

  // Creating a BulkSendApplication and install it on one of the wifi-nodes(except AP)

  uint16_t port = 8808; // random port for TCP server listening.

  // Setting packetsize (Bytes)
  uint32_t packetSize = 1024;
  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (csmaInterfaces.GetAddress (nCsma), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  source.SetAttribute ("SendSize", UintegerValue (packetSize));
  ApplicationContainer sourceApps;
  for (int i = 0; i < int (nWifi); i++)
    {
      sourceApps.Add (source.Install (wifiStaNodes.Get (i)));
    }
  sourceApps.Start (Seconds (2.0));
  sourceApps.Stop (Seconds (2.0 + duration));

  // Creating a PacketSinkApplication and install it on one of the CSMA nodes
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (csmaInterfaces.GetAddress (nCsma), port));
  ApplicationContainer sinkApps = sink.Install (csmaNodes.Get (nCsma));
  sinkApps.Start (Seconds (2.0 - 1.0));
  sinkApps.Stop (Seconds (2.0 + duration));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Initialisation of global variable which are used for Throughput and Delay Calculation.
  data.monitor = data.flowmon.InstallAll ();
  data.totalDelaySum = 0;
  data.totalRxBytes = 0;
  data.totalRxPackets = 0;
  Simulator::Schedule (Seconds (2.0 - 1.0), &Throughput);

  Simulator::Stop (Seconds (2.0 + duration + 1.0));

  if (tracing)
    {
      pointToPoint.EnablePcapAll ("third_p2p");
      phy.EnablePcap ("third_phy", apDevices.Get (0));
      csma.EnablePcap ("third_csma", csmaDevices.Get (0), true);
    }

  Simulator::Run ();
  Simulator::Destroy ();

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  std::cout << "Average Throughput: " << sink1->GetTotalRx () * 8.0 / duration / (1024 * 1024)
            << "Mbps" << std::endl;
  std::cout << "Average Delay: " << data.averageDelay () << "ms" << std::endl;

  return 0;
}