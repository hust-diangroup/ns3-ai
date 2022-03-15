/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Piotr Gawlowicz
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
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 * Based on script: ./examples/tcp/tcp-variants-comparison.cc
 * Modify: Pengyu Liu <eic_lpy@hust.edu.cn>
 *         Hao Yin <haoyin@uw.edu>
 * Modified: Shayekh Bin Islam <shayekh.bin.islam@gmail.com>
 * Topology:
 *
 *   Right Leafs (Clients)                      Left Leafs (Sinks)
 *           |            \                    /        |
 *           |             \    bottleneck    /         |
 *           |              R0--------------R1          |
 *           |             /                  \         |
 *           |   access   /                    \ access |
 *
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <numeric>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
// #include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

#include "ns3/netanim-module.h"

#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

#include "ns3/ns3-ai-module.h"
#include "tcp-rl.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpRl");

static std::vector<uint32_t> rxPkts;

// Flow monitor
FlowMonitorHelper flowHelper;
Ptr<FlowMonitor> monitor;

static void
CountRxPkts (uint32_t sinkId, Ptr<const Packet> packet,
             const Address &srcAddr)
{
  rxPkts[sinkId]++;
}

// static void
// PrintRxCount ()
// {
//   uint32_t size = rxPkts.size ();
//   NS_LOG_UNCOND ("RxPkts:");
//   for (uint32_t i = 0; i < size; i++)
//     {
//       NS_LOG_UNCOND ("---SinkId: " << i << " RxPkts: " << rxPkts.at (i));
//     }
// }


double average(std::vector<double> const& v)
{
  if (v.empty())
  {
    return 0;
  }

  auto const count = static_cast<float>(v.size());
  return std::accumulate(v.begin(), v.end(), 0) * 1.0 / count;
}

int
main (int argc, char *argv[])
{
  SeedManager::SetSeed(21);
  // gSharedMemoryPoolSize
  // LogComponentEnableAll(LOG_LEVEL_ALL);
  // LogComponentEnable ("Pool", LOG_LEVEL_ALL);
  LogComponentEnable ("ns3::TcpRlEnv", LOG_LEVEL_ALL);
  // uint32_t nLeaf = 1;
  uint32_t nLeaf = 4;
  std::string transport_prot = "ns3::TcpRlTimeBased";
  // std::string transport_prot = "ns3::TcpNewReno";
  // double error_p = 0.0;
  // double error_p = 0.01;
  double error_p = 1.0e-3; // 0.1 % = 1e-3
  // std::string bottleneck_bandwidth = "2Mbps";
  std::string bottleneck_bandwidth = "10Mbps";
  // std::string bottleneck_delay = "0.01ms";
  std::string bottleneck_delay = "0.01ms";
  // std::string access_bandwidth = "10Mbps";
  std::string access_bandwidth = "50Mbps";
  // std::string access_delay = "20ms";
  std::string access_delay = "45ms"; // Min-RTT = 180 ms; delay = Min-RTT/4 = 45 ms
  std::string prefix_file_name = "TcpVariantsComparison";
  uint64_t data_mbytes = 0;
  // uint64_t data_mbytes = 100;
  // Maximum transmission unit in bytes to set for the device
  // uint32_t mtu_bytes = 400;
  // Packet size from BDP -> 10 kb = 150 Packets, per pack = 10kb/8 = 1250 B
  uint32_t mtu_bytes = 1250;
  uint32_t max_packets = 50; // L
  // double duration = 5.0;
  // double duration = 20.0;
  // double duration = 50.0;
  double duration = 100.0;
  // double duration = 400.0; // paper
  // double duration = 1000.0;
  uint32_t run = 0;
  // bool flow_monitor = false;
  bool flow_monitor = true;
  bool sack = true;
  std::string queue_disc_type = "ns3::PfifoFastQueueDisc";
  std::string recovery = "ns3::TcpClassicRecovery";
  CommandLine cmd;
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", run);
  // other parameters
  cmd.AddValue ("nLeaf", "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("transport_prot",
                "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
                "TcpLp, TcpRl, TcpRlTimeBased",
                transport_prot);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("bottleneck_bandwidth", "Bottleneck bandwidth",
                bottleneck_bandwidth);
  cmd.AddValue ("bottleneck_delay", "Bottleneck delay", bottleneck_delay);
  cmd.AddValue ("access_bandwidth", "Access link bandwidth",
                access_bandwidth);
  cmd.AddValue ("access_delay", "Access link delay", access_delay);
  cmd.AddValue ("prefix_name", "Prefix of output trace file",
                prefix_file_name);
  cmd.AddValue ("data", "Number of Megabytes of data to transmit",
                data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("duration", "Time to allow flows to run in seconds",
                duration);
  cmd.AddValue ("run", "Run index (for setting repeatable seeds)", run);
  cmd.AddValue ("flow_monitor", "Enable flow monitor", flow_monitor);
  cmd.AddValue ("queue_disc_type",
                "Queue disc type for gateway (e.g. ns3::CoDelQueueDisc)",
                queue_disc_type);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("recovery",
                "Recovery algorithm type to use (e.g., ns3::TcpPrrRecovery", recovery);
  cmd.Parse (argc, argv);
  SeedManager::SetSeed (1);
  SeedManager::SetRun (run);
  NS_LOG_UNCOND ("--seed: " << run);
  NS_LOG_UNCOND ("--Tcp version: " << transport_prot);
  //   // OpenGym Env --- has to be created before any other thing
  //   Ptr<OpenGymInterface> openGymInterface;
  //   if (transport_prot.compare ("ns3::TcpRl") == 0)
  //     {
  //       openGymInterface = OpenGymInterface::Get (openGymPort);
  //       Config::SetDefault ("ns3::TcpRl::Reward",
  //                           DoubleValue (2.0)); // Reward when increasing congestion window
  //       Config::SetDefault ("ns3::TcpRl::Penalty",
  //                           DoubleValue (-30.0)); // Penalty when decreasing congestion window
  //     }
  // if (transport_prot.compare ("ns3::TcpRlTimeBased") == 0)
  //   {
  //     Config::SetDefault ("ns3::TcpRlTimeBased::StepTime",
  //                         TimeValue (Seconds (0.01))); // Time step of TCP env
  //   }
  // Calculate the ADU size
  // ADU is application data unit
  Header *temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);
  // Set the simulation start and stop time
  double start_time = 0.1;
  double stop_time = start_time + duration;
  // 4 MB of TCP buffer
  // Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  // Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize",
                      UintegerValue (mtu_bytes * max_packets));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize",
                      UintegerValue (mtu_bytes * max_packets));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName (recovery)));

  // Select TCP variant
  if (transport_prot.compare ("ns3::TcpWestwoodPlus") == 0)
  {
    // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                        TypeIdValue (TcpWestwood::GetTypeId ()));
    // the default protocol type in ns3::TcpWestwood is WESTWOOD
    Config::SetDefault ("ns3::TcpWestwood::ProtocolType",
                        EnumValue (TcpWestwood::WESTWOODPLUS));
  }
  else
  {
    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot,
                         &tcpTid),
                         "TypeId " << transport_prot << " not found");
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                        TypeIdValue (TypeId::LookupByName (transport_prot)));
  }

  // Configure the error model
  // Here we use RateErrorModel with packet error rate
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);
  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute ("DataRate",
                                     StringValue (bottleneck_bandwidth));
  bottleNeckLink.SetChannelAttribute ("Delay",
                                      StringValue (bottleneck_delay));
  bottleNeckLink.SetDeviceAttribute  ("ReceiveErrorModel",
                                      PointerValue (&error_model));
  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute ("DataRate",
                                       StringValue (access_bandwidth));
  pointToPointLeaf.SetChannelAttribute ("Delay", StringValue (access_delay));
  PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf, nLeaf,
                                pointToPointLeaf, bottleNeckLink);
  // Install IP stack
  InternetStackHelper stack;
  stack.InstallAll ();
  // Traffic Control
  TrafficControlHelper tchPfifo;
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  TrafficControlHelper tchCoDel;
  tchCoDel.SetRootQueueDisc ("ns3::CoDelQueueDisc");
  DataRate access_b (access_bandwidth);
  DataRate bottle_b (bottleneck_bandwidth);
  Time access_d (access_delay);
  Time bottle_d (bottleneck_delay);
  uint32_t size = static_cast<uint32_t> ((std::min (access_b,
                                          bottle_b).GetBitRate () / 8) *
                                         ((access_d + bottle_d + access_d) * 2).GetSeconds ());
  std::cout << "ns3::PfifoFastQueueDisc::MaxSize: " << size / mtu_bytes <<
            "\n";
  // return 0;
  Config::SetDefault ("ns3::PfifoFastQueueDisc::MaxSize",
                      QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, size / mtu_bytes)));
  Config::SetDefault ("ns3::CoDelQueueDisc::MaxSize",
                      QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, size)));

  if (queue_disc_type.compare ("ns3::PfifoFastQueueDisc") == 0)
  {
    tchPfifo.Install (d.GetLeft ()->GetDevice (1));
    tchPfifo.Install (d.GetRight ()->GetDevice (1));
  }
  else if (queue_disc_type.compare ("ns3::CoDelQueueDisc") == 0)
  {
    tchCoDel.Install (d.GetLeft ()->GetDevice (1));
    tchCoDel.Install (d.GetRight ()->GetDevice (1));
  }
  else
  {
    NS_FATAL_ERROR ("Queue not recognized. Allowed values are ns3::CoDelQueueDisc or "
                    "ns3::PfifoFastQueueDisc");
  }

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));
  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // Install apps in left and right nodes
  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;

  for (uint32_t i = 0; i < d.RightCount (); ++i)
  {
    sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    sinkApps.Add (sinkHelper.Install (d.GetRight (i)));
  }

  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (stop_time));

  for (uint32_t i = 0; i < d.LeftCount (); ++i)
  {
    // BulkSendApplication: always-on
    AddressValue remoteAddress (InetSocketAddress (d.GetRightIpv4Address (i), port));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    ftp.SetAttribute ("Remote", remoteAddress);
    ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
    ftp.SetAttribute ("MaxBytes", UintegerValue (data_mbytes * 1000000));
    ApplicationContainer clientApp = ftp.Install (d.GetLeft (i));
    clientApp.Start (Seconds (start_time * i)); // Start after sink
    clientApp.Stop (Seconds (stop_time - 3)); // Stop before the sink
  }

  

  if (flow_monitor)
  {
    monitor = flowHelper.InstallAll ();
  }

  // Count RX packets
  for (uint32_t i = 0; i < d.RightCount (); ++i)
  {
    rxPkts.push_back (0);
    Ptr<PacketSink> pktSink = DynamicCast<PacketSink> (sinkApps.Get (i));
    pktSink->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&CountRxPkts, i));
  }

  Simulator::Stop (Seconds (stop_time));
  // NS_LOG_UNCOND("Tracing enabled\n");
  // AsciiTraceHelper asciiTrace;
  // pointToPointLeaf.EnableAsciiAll(asciiTrace.CreateFileStream("pointToPointLeaf.tr"));
  // bottleNeckLink.EnableAsciiAll(asciiTrace.CreateFileStream("bottleNeckLink.tr"));
  // pointToPointLeaf.EnablePcapAll("pointToPointLeaf");
  // bottleNeckLink.EnablePcapAll("bottleNeckLink", true);
  // bottleNeckLink.EnablePcap("bottleNeckLink.cap", d.GetLeft ()->GetDevice (1), 1);
  d.BoundingBox (1, 1, 100, 100);
  std::string animFile = "dumbbell-animation.xml";  // Name of file for animation output
  AnimationInterface anim (animFile);
  anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10)); // Optional
  Simulator::Run ();

  if (flow_monitor)
  {
    // 10. Print per flow statistics
    // example/wireless/wifi-hidden-terminal.cc
    // traffic-control.cc
    // https://www.nsnam.org/tutorials/consortium13/visualization-tutorial.pdf
    // https://groups.google.com/g/ns-3-users/c/vs5UYGraEDw
    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    std::ofstream flow_out;
    flow_out.open("flow.txt");
    double total_tp = 0.0, total_delay = 0.0;
    int cnt = 0;

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      // i->second.delaySum;
      // d.GetLeft(0)->
      flow_out << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
      flow_out << "  Tx Packets: " << i->second.txPackets << "\n";
      flow_out << "  Tx Bytes:   " << i->second.txBytes << "\n";
      // flow_out << "  TxOffered:  " << i->second.txBytes * 8.0 / <time> / 1000 / 1000  << " Mbps\n";
      flow_out << "  Rx Packets: " << i->second.rxPackets << "\n";
      flow_out << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      flow_out << "  Lost Packets:   " << i->second.lostPackets << "\n";
      flow_out << "  Packet loss rate: " << (i->second.lostPackets / (float)i->second.txPackets) << " %\n";
      // ns3::int64x64_t delay = i->second.delaySum.GetMilliSeconds() / i->second.rxPackets;
      // flow_out << "  Delay: " << std::setprecision(6) << delay.GetDouble() << " ms\n";
      Time temp_time(access_delay);
      Time temp_time2(bottleneck_delay);
      ns3::int64x64_t delay = (i->second.delaySum.GetMilliSeconds() /
                               i->second.rxPackets) \
                              - (2 * temp_time.GetMilliSeconds() + temp_time2.GetMilliSeconds());
      flow_out << "  Delay: " << std::setprecision(6) << delay.GetDouble() << " ms\n";
      ns3::int64x64_t throughput = (i->second.rxBytes * 8.0 /
                                    (i->second.timeLastRxPacket
                                     - i->second.timeFirstTxPacket).GetSeconds()) / 1000 / 1000;
      flow_out << "  Throughput (tp): " << std::setprecision(6) << throughput.GetDouble() << " Mbps (M = 1000000)\n";

      // double thr = 0;
      // uint64_t totalPacketsThr = DynamicCast<PacketSink> (sinkApps.Get ((i->first-1)/2))->GetTotalRx ();
      // thr = totalPacketsThr * 8 / (stop_time * 1000000.0); //Mbit/s
      // std::cout << "  Rx Bytes: " << totalPacketsThr << std::endl;
      // std::cout << "  Average Goodput: " << thr << " Mbit/s" << std::endl;

      // i->second.delaySum.To(ns3::Time::S);

      // i->second.delayHistogram.SerializeToXmlStream(std::cout, 2, "");
      // std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / <time> / 1000 / 1000  << " Mbps\n";

      if (i->first % 2 == 1)
      {
        // delays.push_back(delay.GetDouble());
        // tps.push_back(throughput.GetDouble());
        total_tp += throughput.GetDouble();
        total_delay += delay.GetDouble();
        cnt += 1;
      }
    }

    flow_out << "Average tp: " << std::setprecision(6) << (total_tp / cnt) << "\n";
    flow_out << "Average delay: " << std::setprecision(6) << (total_delay / cnt) << "\n";
    flow_out.close();
  }

  if (flow_monitor)
  {
    flowHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", true, true);
  }

  //   if (transport_prot.compare ("ns3::TcpRl") == 0 or transport_prot.compare ("ns3::TcpRlTimeBased") == 0)
  //     {
  //       openGymInterface->NotifySimulationEnd ();
  //     }
  // PrintRxCount ();
  std::cout << "Animation Trace file created:" << animFile.c_str () << std::endl;
  Simulator::Destroy ();
  return 0;
}
