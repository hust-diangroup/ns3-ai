#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("LenaSimpleEpc");

int
main (int argc, char *argv[])
{
  // LogComponentEnable ("MyRrMacScheduler", LOG_LEVEL_INFO);
  
  // Number of Users
  uint16_t m_nUser = 4;
  // Distance
  double distance = 600;

  double speed = 10;

  string datarate = "20Mbps";
  uint32_t packetSize = 1200;

  // Set the simulation time
  double simTime = 5.0;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numberOfNodes", "Number of eNodeBs + UE pairs", m_nUser);
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue ("datarate", "datarate", datarate);
  cmd.AddValue ("packetSize", "packetSize", packetSize);
  cmd.AddValue ("speed", "speed", speed);
  // cmd.AddValue ("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse (argc, argv);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

  RngSeedManager::SetSeed (6);
  RngSeedManager::SetRun (4);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::MyRrMacScheduler");
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (1);
  ueNodes.Create (m_nUser);

  // Install Mobility Model
  Ptr<ListPositionAllocator> apPositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> staPositionAlloc = CreateObject<ListPositionAllocator> ();
  apPositionAlloc->Add (Vector (0, 0, 25));
  staPositionAlloc->Add (Vector (distance, 368, 1.5));
  staPositionAlloc->Add (Vector (10, 400, 1.5));
  staPositionAlloc->Add (Vector (10, 400, 1.5));
  staPositionAlloc->Add (Vector (10, 400, 1.5));
  staPositionAlloc->Add (Vector (10, 400, 1.5));
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (apPositionAlloc);
  mobility.Install (enbNodes);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.SetPositionAllocator (staPositionAlloc);
  mobility.Install (ueNodes);

  Vector sp (speed, 0, 0);
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (sp);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  Ptr<LteEnbNetDevice> lteEnbDev = enbLteDevs.Get (0)->GetObject<LteEnbNetDevice> ();
  Ptr<LteEnbPhy> enbPhy = lteEnbDev->GetPhy ();
  enbPhy->SetAttribute ("TxPower", DoubleValue (30.0));
  enbPhy->SetAttribute ("NoiseFigure", DoubleValue (5.0));

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < m_nUser; i++)
    {
      lteHelper->Attach (ueLteDevs.Get (i), enbLteDevs.Get (0));
      // side effect: the default EPS bearer will be activated
    }

  Time udpInterval = Time::FromDouble (
      (packetSize * 8) / static_cast<double> (DataRate (datarate).GetBitRate ()), Time::S);

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      dlPort++;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                           InetSocketAddress (Ipv4Address::GetAny (), dlPort));

      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (u)));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
      dlClient.SetAttribute ("Interval", TimeValue (udpInterval));
      dlClient.SetAttribute ("MaxPackets", UintegerValue (0xFFFFFFFF));

      clientApps.Add (dlClient.Install (remoteHost));
    }

  serverApps.Start (MilliSeconds (10));
  clientApps.Start (MilliSeconds (10));
  lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-simple-epc");

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  // flowmon.Install (enbNodes.Get (0));

  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (simTime)));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  cout << "Start Simulation" << endl;

  monitor->CheckForLostPackets ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  double Throughput = 0.0;

  for (map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end ();
       ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      cout << "Flow ID: " << i->first << " Src Addr " << t.sourceAddress << " Dst Addr "
           << t.destinationAddress << endl;
      cout << "Tx Packets = " << i->second.txPackets << endl;
      cout << "Rx Packets = " << i->second.rxPackets << endl;
      Throughput =
          i->second.rxBytes * 8.0 /
          (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) /
          1024;
      cout << "Throughput: " << Throughput << " Kbps" << endl;
    }

  NS_LOG_UNCOND ("Done");

  cout << "Simulation End" << endl;

  Simulator::Destroy ();
  return 0;
}
