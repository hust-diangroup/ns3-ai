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

#include "multi-bss.h"

#include "tgax-residential-propagation-loss-model.h"

#include "ns3/ai-module.h"
#include "ns3/ampdu-subframe-header.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/buildings-helper.h"
#include "ns3/buildings-module.h"
#include "ns3/burst-sink-helper.h"
#include "ns3/bursty-helper.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/ctrl-headers.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/he-configuration.h"
#include "ns3/he-phy.h"
#include "ns3/ht-phy.h"
#include "ns3/integer.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/queue-item.h"
#include "ns3/queue-size.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/seq-ts-size-frag-header.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/ssid.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/threshold-preamble-detection-model.h"
#include "ns3/trace-file-burst-generator.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
// #include "ns3/v4ping-helper.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>

/// Avoid std::numbers::pi because it's C++20
#define PI 3.1415926535

#define N_BSS 4

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("multi-bss");
Ptr<UniformRandomVariable> randomX = CreateObject<UniformRandomVariable>();
Ptr<UniformRandomVariable> randomY = CreateObject<UniformRandomVariable>();

double distance = 0.001; ///< The distance in meters between the AP and the STAs
bool drlCca = false;
double duration = 0;
int totalDropsByOverlap = 0;
uint8_t boxSize = 10;
std::map<int, std::string> configuration;
uint32_t pktSize = 1500; ///< packet size used for the simulation (in bytes)
uint8_t maxMpdus = 0;    ///< The maximum number of MPDUs in A-MPDUs (0 to disable MPDU aggregation)
std::string appType("constant");
std::map<Mac48Address, Time>
    timeFirstReceived; ///< Map that stores the time at which the first packet was received per AP
///< (and the packet is addressed to that AP)
std::map<Mac48Address, Time>
    timeLastReceived; ///< Map that stores the time at which the last packet was received per AP
///< (and the packet is addressed to that AP)
std::map<Mac48Address, uint64_t> packetsReceived; ///< Map that stores the total packets received
                                                  ///< per AP (and addressed to that AP)
std::map<Mac48Address, uint64_t>
    bytesReceived; ///< Map that stores the total bytes received per AP (and addressed to that AP)
std::map<uint32_t, uint64_t> intervalBytesReceived;
std::map<uint32_t, std::vector<double>> intervalEdcaHolSample;
std::map<uint32_t, std::vector<double>> edcaHolSample;
uint32_t networkSize;
NetDeviceContainer apDevices;
NetDeviceContainer staDevices;
NetDeviceContainer devices;
NodeContainer wifiNodes;
NodeContainer apNodes;
NodeContainer staNodes;
int apNodeCount = 0;
double txPower; ///< The transmit power of all the nodes in dBm
double ccaSensitivity;
std::string propagationModel = "";

std::map<uint32_t, std::vector<double>> nodeCw;
std::map<uint32_t, std::vector<double>> nodeBackoff;
std::map<uint64_t, int> dataRateToMcs;
std::map<uint32_t, int> nodeMcs;

std::vector<std::string>
csv_split(std::string source, char delimeter)
{
    std::vector<std::string> ret;
    std::string word = "";
    //    int start = 0;

    bool inQuote = false;
    for (uint32_t i = 0; i < source.size(); ++i)
    {
        if (!inQuote && source[i] == '"')
        {
            inQuote = true;
            continue;
        }
        if (inQuote && source[i] == '"')
        {
            if (source.size() > i && source[i + 1] == '"')
            {
                ++i;
            }
            else
            {
                inQuote = false;
                continue;
            }
        }

        if (!inQuote && source[i] == delimeter)
        {
            ret.push_back(word);
            word = "";
        }
        else
        {
            word += source[i];
        }
    }
    ret.push_back(word);

    return ret;
}

// Function object to compute the hash of a MAC address
struct MacAddressHash
{
    std::size_t operator()(const Mac48Address& address) const;
};

std::size_t
MacAddressHash::operator()(const Mac48Address& address) const
{
    uint8_t buffer[6];
    address.CopyTo(buffer);
    std::string s(buffer, buffer + 6);
    return std::hash<std::string>{}(s);
}

std::unordered_map<Mac48Address, uint32_t, MacAddressHash> m_staMacAddressToNodeId;

const std::map<AcIndex, std::string> m_aciToString = {
    {AC_BE, "BE"},
    {AC_BK, "BK"},
    {AC_VI, "VI"},
    {AC_VO, "VO"},
};

struct InFlightPacketInfo
{
    Mac48Address m_srcAddress;
    Mac48Address m_dstAddress;
    AcIndex m_ac;
    Ptr<const Packet> m_ptrToPacket;
    Time m_edcaEnqueueTime{Seconds(0)}; // time the packet was enqueued into an EDCA queue
    Time m_edcaDequeueTime{Seconds(0)}; // time the packet was dequeued from EDCA queue
    Time appTypeTxTime{Seconds(0)};     // time the packet was created by app (E2E)
    Time appTypeRxTime{Seconds(0)};     // time the packet was received by app (E2E)
    Time m_HoLTime{Seconds(0)};         // time the packet became Head of Line
    Time m_L2RxTime{Seconds(0)};  // time packet got forwarded up to the Mac Layer (L2 latency =
                                  // L2RxTime - edcaEnqueueTime)
    Time m_phyTxTime{Seconds(0)}; // time packet began transmission
    bool m_dequeued{false};
};

std::unordered_map<uint64_t /* UID */, std::list<InFlightPacketInfo>> m_inFlightPacketMap;

uint32_t
MacAddressToNodeId(Mac48Address address)
{
    for (uint32_t i = 0; i < apDevices.GetN(); i++)
    {
        if (apDevices.Get(i)->GetAddress() == address)
        {
            return apNodes.Get(i)->GetId();
        }
    }

    auto it = m_staMacAddressToNodeId.find(address);
    if (it != m_staMacAddressToNodeId.end())
    {
        return it->second;
    }

    NS_ABORT_MSG("Found no node having MAC address " << address);
}

void
NotifyEdcaEnqueue(Ptr<const WifiMpdu> item)
{
    if (!item->GetHeader().IsQosData())
    {
        return;
    }
    // std::cout << "Enqueue UID " << item->GetPacket()->GetUid() << "  " << Simulator::Now()
    //           << std::endl;
    // init a map entry if the packet's UID is not present
    auto mapIt =
        m_inFlightPacketMap.insert({item->GetPacket()->GetUid(), std::list<InFlightPacketInfo>()})
            .first;

    InFlightPacketInfo info;
    info.m_srcAddress = item->GetHeader().GetAddr2();
    info.m_dstAddress = item->GetHeader().GetAddr1();
    info.m_ptrToPacket = item->GetPacket();
    info.m_edcaEnqueueTime = Simulator::Now();

    mapIt->second.insert(mapIt->second.end(), info);
}

void
NotifyAppTx(Ptr<const Packet> packet, const Address& address)
{
    // std::cout << "App Tx UID " << packet->GetUid() << std::endl;
    Ptr<const Packet> p = packet;

    auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    if (mapIt == m_inFlightPacketMap.end())
    {
        // std::cout << "No packet with UID " << p->GetUid() << " is currently in queue" <<
        // std::endl;
        return;
    }

    auto listIt =
        std::find_if(mapIt->second.begin(),
                     mapIt->second.end(),
                     [&p](const InFlightPacketInfo& info) { return info.m_ptrToPacket == p; });

    if (listIt == mapIt->second.end())
    {
        // std::cout << "Forwarding up a packet that has not been enqueued?" << std::endl;
        return;
    }

    InFlightPacketInfo info;
    info.m_srcAddress = listIt->m_srcAddress;
    info.m_dstAddress = listIt->m_dstAddress;
    info.m_ptrToPacket = packet;
    info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    info.m_edcaDequeueTime = listIt->m_edcaDequeueTime;
    info.appTypeTxTime = Simulator::Now();

    mapIt->second.erase(listIt);
    mapIt->second.insert(mapIt->second.end(), info);
}

/**
 * Parse context strings of the form "/NodeList/x/DeviceList/x/..." to extract the NodeId
 * integer
 *
 * \param context The context to parse.
 * \return the NodeId
 */
uint32_t
ContextToNodeId(std::string context)
{
    std::string sub = context.substr(10);
    uint32_t pos = sub.find("/Device");
    return std::stoi(sub.substr(0, pos));
}

std::unordered_map<uint32_t, std::map<uint64_t, std::vector<Time>>> nodePacketTxTime;
std::unordered_map<uint32_t, std::map<uint64_t, std::vector<Time>>> nodePacketTxEndTime;
int totalTx = 0;

void
NotifyPhyTxBegin(std::string context, Ptr<const Packet> p, double txPowerW)
{
    totalTx += 1;
    nodePacketTxTime[ContextToNodeId(context)][p->GetUid()].push_back(Simulator::Now());

    auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    if (mapIt == m_inFlightPacketMap.end())
    {
        return;
    }

    auto listIt = std::find_if(mapIt->second.begin(),
                               mapIt->second.end(),
                               [&p](const InFlightPacketInfo& info) {
                                   return info.m_ptrToPacket->GetUid() == p->GetUid();
                               });

    if (listIt == mapIt->second.end())
    {
        // std::cout << "Forwarding up a packet that has not been enqueued?" << std::endl;
        return;
    }

    InFlightPacketInfo info;
    info.m_srcAddress = listIt->m_srcAddress;
    info.m_dstAddress = listIt->m_dstAddress;
    info.m_ptrToPacket = listIt->m_ptrToPacket;
    info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    info.appTypeTxTime = listIt->appTypeTxTime;
    info.m_phyTxTime = Simulator::Now();

    mapIt->second.erase(listIt);
    mapIt->second.insert(mapIt->second.end(), info);
}

/**
 * PHY TX end trace.
 *
 * \param context The context.
 * \param p The packet.
 */
void
PhyTxDoneTrace(std::string context, Ptr<const Packet> p)
{
    nodePacketTxEndTime[ContextToNodeId(context)][p->GetUid()].push_back(Simulator::Now());
}

/**
 * PHY TX drop trace.
 *
 * \param context The context.
 * \param p The packet.
 */
void
PhyTxDropTrace(std::string context, Ptr<const Packet> p)
{
    nodePacketTxEndTime[ContextToNodeId(context)][p->GetUid()].push_back(Simulator::Now());
}

void
NotifyMacForwardUp(Ptr<const Packet> p)
{
    // std::cout << "MacForwardUp UID " << p->GetUid() << std::endl;
    // Ptr<const Packet> p = packet;

    auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    if (mapIt == m_inFlightPacketMap.end())
    {
        // std::cout << "No packet with UID " << p->GetUid() << " is currently in queue" <<
        // std::endl;
        return;
    }

    auto listIt = std::find_if(mapIt->second.begin(),
                               mapIt->second.end(),
                               [&p](const InFlightPacketInfo& info) {
                                   return info.m_ptrToPacket->GetUid() == p->GetUid();
                               });

    if (listIt == mapIt->second.end())
    {
        std::cout << "Forwarding up a packet that has not been enqueued?" << std::endl;
        return;
    }
    if (listIt->m_dstAddress.IsGroup())
    {
        return;
    }

    InFlightPacketInfo info;
    info.m_srcAddress = listIt->m_srcAddress;
    info.m_dstAddress = listIt->m_dstAddress;
    info.m_ptrToPacket = listIt->m_ptrToPacket;
    info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    info.appTypeTxTime = listIt->appTypeTxTime;
    info.m_phyTxTime = listIt->m_phyTxTime;
    // info.appTypeRxTime = Simulator::Now();
    info.m_L2RxTime = Simulator::Now();

    mapIt->second.erase(listIt);
    mapIt->second.insert(mapIt->second.end(), info);
}

int appTxrec = 0;

void
NotifyAppRx(Ptr<const Packet> packet, const Address& address)
{
    // std::cout << "App Rx UID " << packet->GetUid() << std::endl;

    Ptr<const Packet> p = packet;

    auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    if (mapIt == m_inFlightPacketMap.end())
    {
        // std::cout << "No packet with UID " << p->GetUid() << " is currently in queue" <<
        // std::endl;
        return;
    }
    // mapIt->second.;
    auto listIt = std::find_if(mapIt->second.begin(),
                               mapIt->second.end(),
                               [&p](const InFlightPacketInfo& info) {
                                   return info.m_ptrToPacket->GetUid() == p->GetUid();
                               });

    if (listIt == mapIt->second.end())
    {
        std::cout << "Forwarding up a packet that has not been enqueued?" << std::endl;
        return;
    }
    InFlightPacketInfo info;
    info.m_srcAddress = listIt->m_srcAddress;
    info.m_dstAddress = listIt->m_dstAddress;
    info.m_ptrToPacket = listIt->m_ptrToPacket;
    info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    info.appTypeTxTime = listIt->appTypeTxTime;
    info.m_phyTxTime = listIt->m_phyTxTime;
    info.m_L2RxTime = listIt->m_L2RxTime;
    info.appTypeRxTime = Simulator::Now();

    appTxrec++;
    // std::cout << "APRX" << std::endl;

    mapIt->second.erase(listIt);
    mapIt->second.insert(mapIt->second.end(), info);
}

std::map<uint32_t, Time> dequeueTimes;

void
NotifyMsduDequeuedFromEdcaQueue(Ptr<const WifiMpdu> item)
{
    // std::cout << "Dequeue UID " << item->GetPacket()->GetUid() << std::endl;

    if (!item->GetHeader().IsQosData() || item->GetHeader().GetAddr1().IsGroup())
    {
        // the frame is not a unicast QoS data frame or the MSDU lifetime is higher than the
        // max queue delay, hence the MSDU has been discarded. Do nothing in this case.
        return;
    }
    Ptr<const Packet> p = item->GetPacket();

    uint64_t srcNodeId = MacAddressToNodeId(item->GetHeader().GetAddr2());
    auto iter = dequeueTimes.find(srcNodeId);
    if (iter == dequeueTimes.end())
    {
        dequeueTimes.insert(std::make_pair(srcNodeId, Simulator::Now()));
        return;
    }
    if (iter->second == Simulator::Now())
    {
        // std::cout << "last " << iter->second << " now " << Simulator::Now() << std::endl;
        return;
    }

    auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    if (mapIt == m_inFlightPacketMap.end())
    {
        // std::cout << "No packet with UID " << p->GetUid() << " is currently in queue" <<
        // std::endl;
        return;
    }

    auto listIt =
        std::find_if(mapIt->second.begin(),
                     mapIt->second.end(),
                     [&p](const InFlightPacketInfo& info) { return info.m_ptrToPacket == p; });

    if (listIt == mapIt->second.end())
    {
        // std::cout << "Dequeue a packet that has not been enqueued?" << std::endl;
        return;
    }

    InFlightPacketInfo info;
    info.m_srcAddress = item->GetHeader().GetAddr2();
    info.m_dstAddress = item->GetHeader().GetAddr1();
    info.m_ptrToPacket = item->GetPacket();
    info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    info.m_edcaDequeueTime = Simulator::Now();
    info.appTypeTxTime = listIt->appTypeTxTime;
    info.m_phyTxTime = listIt->m_phyTxTime;
    info.appTypeRxTime = listIt->appTypeRxTime;
    info.m_L2RxTime = listIt->m_L2RxTime;
    info.m_HoLTime = std::max(listIt->m_edcaEnqueueTime, iter->second);
    info.m_dequeued = true;

    // HERE CALCULATE ALL DELAYS
    double newHolSample = (info.m_edcaDequeueTime - info.m_HoLTime).ToDouble(Time::MS);
    double queingDelay = (info.m_HoLTime - info.m_edcaEnqueueTime).ToDouble(Time::MS);
    double accessDelay = (info.m_phyTxTime - info.m_HoLTime).ToDouble(Time::MS);
    double txDelay = (info.m_edcaDequeueTime - info.m_phyTxTime).ToDouble(Time::MS);

    auto it = edcaHolSample.find(srcNodeId);
    auto ite = intervalEdcaHolSample.find(srcNodeId);
    if (ite == intervalEdcaHolSample.end())
    {
        std::vector<double> tiVec;
        tiVec.push_back(newHolSample);
        tiVec.push_back(queingDelay);
        tiVec.push_back(accessDelay);
        tiVec.push_back(txDelay);
        intervalEdcaHolSample.insert(std::make_pair(srcNodeId, tiVec));
        // std::cout << "first Sample: " << newHolSample << "\n";
    }
    else
    {
        ite->second.push_back(newHolSample);
        ite->second.push_back(queingDelay);
        ite->second.push_back(accessDelay);
        ite->second.push_back(txDelay);
    }
    if (it == edcaHolSample.end())
    {
        std::vector<double> tiVec;
        tiVec.push_back(newHolSample);
        tiVec.push_back(queingDelay);
        tiVec.push_back(accessDelay);
        tiVec.push_back(txDelay);
        edcaHolSample.insert(std::make_pair(srcNodeId, tiVec));
        // std::cout << "first Sample: " << newHolSample << "\n";
    }
    else
    {
        it->second.push_back(newHolSample);
        it->second.push_back(queingDelay);
        it->second.push_back(accessDelay);
        it->second.push_back(txDelay);
    }

    iter->second = Simulator::Now();
    mapIt->second.erase(listIt);
    mapIt->second.insert(mapIt->second.end(), info);
}

void
NotifyMsduDroppedAfterDequeueFromEdcaQueue(Ptr<const WifiMpdu> item)
{
    // std::cout << "did it work?" << std::endl;
}

void
StartStatistics()
{
    for (auto devIt = devices.Begin(); devIt != devices.End(); devIt++)
    {
        Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice>(*devIt);
        // trace packets forwarded up by the MAC layer
        dev->GetMac()->TraceConnectWithoutContext("MacRx", MakeCallback(&NotifyMacForwardUp));

        for (auto& ac : m_aciToString)
        {
            dev->GetMac()->GetTxopQueue(ac.first)->TraceConnectWithoutContext(
                "Enqueue",
                MakeBoundCallback(&NotifyEdcaEnqueue));
            dev->GetMac()->GetTxopQueue(ac.first)->TraceConnectWithoutContext(
                "Dequeue",
                MakeCallback(&NotifyMsduDequeuedFromEdcaQueue));
            dev->GetMac()->GetTxopQueue(ac.first)->TraceConnectWithoutContext(
                "DropAfterDequeue",
                MakeCallback(&NotifyMsduDroppedAfterDequeueFromEdcaQueue));
        }
    }
}

/**
 * Incremement the counter for a given address.
 *
 * \param [out] counter The counter to increment.
 * \param addr The address to incremement the counter for.
 * \param increment The incremement (1 if omitted).
 */
void
IncrementCounter(std::map<Mac48Address, uint64_t>& counter,
                 Mac48Address addr,
                 uint64_t increment = 1)
{
    auto it = counter.find(addr);
    if (it != counter.end())
    {
        it->second += increment;
    }
    else
    {
        counter.insert(std::make_pair(addr, increment));
    }
}

uint32_t associatedStas = 0;
uint32_t deassociatedStas = 0;

/**
 * Reset the stats.
 */
void
RestartCalc()
{
    bytesReceived.clear();
    packetsReceived.clear();
    timeFirstReceived.clear();
    timeLastReceived.clear();
    appTxrec = 0;
    if (associatedStas < staNodes.GetN())
    {
        NS_ABORT_MSG("Not all STA associated. Missing: " << (staNodes.GetN() - associatedStas));
    }
}

// void
// TrackTime()
//{
//     std::cout << "Time = " << Simulator::Now().GetSeconds() << "s" << std::endl;
//     Simulator::Schedule(Seconds(5), &TrackTime);
// }

void
CheckAssociation()
{
    if (associatedStas < staNodes.GetN())
    {
        // NS_ABORT_MSG("Not all STA associated. Missing: " << (staNodes.GetN() -
        // associatedStas));
        std::cout << "RESTARTED ASSOCIATION" << std::endl;
        for (uint32_t i = 0; i < staNodes.GetN(); i++)
        {
            Ptr<NetDevice> dev = staNodes.Get(i)->GetDevice(0);
            Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);

            Ptr<WifiPhy> dev_phy = wifi_dev->GetPhy();

            Ptr<WifiMac> q_mac = wifi_dev->GetMac();

            Ptr<StaWifiMac> staMac = StaticCast<StaWifiMac>(q_mac);

            if (!(staMac->IsAssociated()))
            {
                staMac->ScanningTimeout(std::nullopt);
            }
        }
        Simulator::Schedule(Seconds(1), &CheckAssociation);
    }
    else
    {
        std::cout << "associated N Sta: " << associatedStas << std::endl;
    }
}

/**
 * Parse context strings of the form "/NodeList/x/DeviceList/x/..." and fetch the Mac address
 *
 * \param context The context to parse.
 * \return the device MAC address
 */
Mac48Address
ContextToMac(std::string context)
{
    std::string sub = context.substr(10);
    uint32_t pos = sub.find("/Device");
    uint32_t nodeId = std::stoi(sub.substr(0, pos));
    Ptr<Node> n = NodeList::GetNode(nodeId);
    Ptr<WifiNetDevice> d;
    for (uint32_t i = 0; i < n->GetNDevices(); i++)
    {
        d = n->GetDevice(i)->GetObject<WifiNetDevice>();
        if (d)
        {
            break;
        }
    }
    return Mac48Address::ConvertFrom(d->GetAddress());
}

std::map<uint32_t, std::map<uint32_t, double>> nodeRxPower;

void
GetRxPower(Ptr<TgaxResidentialPropagationLossModel> tgaxPropModel)
{
    for (uint32_t i = 0; i < wifiNodes.GetN(); i++)
    {
        Ptr<NetDevice> dev = wifiNodes.Get(i)->GetDevice(0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        Ptr<Object> object = wifiNodes.Get(i);
        Ptr<MobilityModel> model1 = object->GetObject<MobilityModel>();
        Ptr<WifiPhy> wifi_phy = wifi_dev->GetPhy();

        for (uint32_t x = 0; x < wifiNodes.GetN(); x++)
        {
            // Skip same nodes
            if (i == x)
            {
                continue;
            }

            Ptr<NetDevice> dev2 = wifiNodes.Get(x)->GetDevice(0);
            Ptr<WifiNetDevice> wifi_dev2 = DynamicCast<WifiNetDevice>(dev2);
            Ssid ssid2 = wifi_dev2->GetMac()->GetSsid();
            // Receiver must be node in BSS-0
            if (!ssid2.IsEqual(Ssid("BSS-0")))
            {
                continue;
            }
            Ptr<Object> object2 = wifiNodes.Get(x);
            Ptr<MobilityModel> model2 = object2->GetObject<MobilityModel>();
            double rxPower = 0;
            for (int j = 0; j < 100; j++)
            {
                rxPower += tgaxPropModel->GetRxPower(wifi_phy->GetTxPowerStart(), model1, model2);
            }
            nodeRxPower[wifiNodes.Get(i)->GetId()][wifiNodes.Get(x)->GetId()] = (rxPower / 100);
        }
    }
}

/**
 * Print the buildings list in a format that can be used by Gnuplot to draw them.
 *
 * \param filename The output filename.
 */
void
PrintPythonPlotCSV(std::string filename)
{
    // for (uint32_t i = 0; i < bytes__Received.size(); i++)
    // {
    //     std::cout << "Bytes received: " << bytes__Received[i] << std::endl;
    // }
    std::ofstream outFile;
    outFile.open(filename.c_str(), std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }
    //    uint32_t index = 0;
    Ptr<Building> building;
    for (BuildingList::Iterator it = BuildingList::Begin(); it != BuildingList::End(); ++it)
    {
        //        ++index;
        Box box = (*it)->GetBoundaries();
        building = *it;
        int rowCount = 1;
        if (apNodeCount > 2)
        {
            rowCount = 2;
        }
        outFile << "box," << box.xMax - box.xMin << "," << box.yMax - box.yMin << "," << rowCount
                << std::endl;
    }
    double maxDistance;
    if (appType != "setup-done")
    { // Formula to draw radius of circle
        double max_loss = -(ccaSensitivity - txPower);
        // double maxDistance = std::pow(10, (max_loss - 40.05 - 20 * std::log10(5e9 / 2.4e9)) /
        // 20);
        maxDistance = pow(10, ((max_loss - 40.05 - 20 * std::log10(5e9 / 2.4e9)) / 20));
        // maxDistance = (max_loss - 40.05 - 20 * std::log10(5e9 / 2.4e9)) / (5 - 4 *
        // std::log10(5));
    }
    // double maxDistance_log = std::pow(10, max_loss / (10 * 3));

    // max_loss = txPower - ccaSensitivity;
    // // double maxDistance2 = std::pow(10, (max_loss - 40.05 - 20 * std::log10(5e9 / 2.4e9)) /
    // // 20);
    // double maxDistance2 =
    //     (max_loss - 40.05 - 20 * std::log10(5e9 / 2.4e9)) / (5 - 4 * std::log10(5));

    for (uint32_t i = 0; i < apNodes.GetN(); i++)
    {
        Ptr<NetDevice> dev = apNodes.Get(i)->GetDevice(0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        Ptr<Object> object = apNodes.Get(i);
        Ptr<MobilityModel> model = object->GetObject<MobilityModel>();
        Vector pos = model->GetPosition();
        Ssid ssid = wifi_dev->GetMac()->GetSsid();
        // Ptr<WifiPhy> wifi_phy = wifi_dev->GetPhy();
        // wifi_phy->SetCcaSensitivityThreshold();

        if (appType == "setup-done")
        {
            // std::string configString = configuration[apNodes.Get(i)->GetId()];

            std::string configString = configuration[apNodes.Get(i)->GetId()];
            std::vector<std::string> configValues = csv_split(configString, ',');
            double m_ccaSensitivity = std::stoi(configValues[1]);
            double m_txPower = std::stoi(configValues[2]);
            //            double m_chWidth = std::stoi(configValues[3]);
            //            double m_chNumber = std::stoi(configValues[4]);

            // size_t pos = configString.find(',');
            // configString = configString.substr(pos + 1);
            // size_t pos2 = configString.find(',');
            //            std::cout << "CCaSensitivity: " << m_ccaSensitivity << std::endl;
            // double m_ccaSensitivity = std::stoi(configString.substr(0, pos2));
            //            std::cout << "apTxPower: " << m_txPower << std::endl;
            // double m_txPower = std::stoi(configString.substr(pos2 + 1));
            // Formula to draw radius of circle
            double max_loss = -(m_ccaSensitivity - m_txPower); // TODO: fix to specific tx power
            // double maxDistance = std::pow(10, (max_loss - 40.05 - 20 * std::log10(5e9
            // / 2.4e9)) / 20);
            maxDistance =
                (max_loss - 40.05 - 20 * std::log10(5e9 / 2.4e9)) / (5 - 4 * std::log10(5));
        }
        outFile << "AP," << pos.x << "," << pos.y << "," << ssid.PeekString() << "-"
                << apNodes.Get(i)->GetId() << "-"
                << "HeMcs" << nodeMcs[apNodes.Get(i)->GetId()] << "," << maxDistance << std::endl;
    }
    for (uint32_t i = 0; i < staNodes.GetN(); i++)
    {
        Ptr<NetDevice> dev = staNodes.Get(i)->GetDevice(0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        Ptr<Object> object = staNodes.Get(i);
        Ptr<MobilityModel> model = object->GetObject<MobilityModel>();
        Vector pos = model->GetPosition();
        Ssid ssid = wifi_dev->GetMac()->GetSsid();

        if (appType == "setup-done")
        {
            std::string configString = configuration[staNodes.Get(i)->GetId()];

            std::vector<std::string> configValues = csv_split(configString, ',');
            double m_ccaSensitivity = std::stoi(configValues[1]);
            double m_txPower = std::stoi(configValues[2]);
            //            double m_chWidth = std::stoi(configValues[3]);
            //            double m_chNumber = std::stoi(configValues[4]);

            // size_t pos = configString.find(',');
            // configString = configString.substr(pos + 1);
            // size_t pos2 = configString.find(',');
            std::cout << "CCaSensitivity: " << m_ccaSensitivity << std::endl;
            // double m_ccaSensitivity = std::stoi(configString.substr(0, pos2));
            std::cout << "apTxPower: " << m_txPower << std::endl;

            // size_t pos = configString.find(',');
            // configString = configString.substr(pos + 1);
            // size_t pos2 = configString.find(',');
            // std::cout << "CCaSensitivity: " << configString.substr(0, pos2) << std::endl;
            // double m_ccaSensitivity = std::stoi(configString.substr(0, pos2));
            // std::cout << "apTxPower: " << configString.substr(pos2 + 1) << std::endl;
            // double m_txPower = std::stoi(configString.substr(pos2 + 1));
            // Formula to draw radius of circle
            double max_loss = -(m_ccaSensitivity - m_txPower); // TODO: fix to specific tx power
            maxDistance =
                (max_loss - 40.05 - 20 * std::log10(5e9 / 2.4e9)) / (5 - 4 * std::log10(5));
        }
        outFile << "STA," << pos.x << "," << pos.y << "," << ssid.PeekString() << "-"
                << staNodes.Get(i)->GetId() << "-"
                << "HeMcs" << nodeMcs[staNodes.Get(i)->GetId()] << "," << maxDistance << std::endl;
    }
}

void
RestartIntervalThroughputHolDelay()
{
    intervalBytesReceived.clear();
    // std::cout << "Amount of samples " << intervalEdcaHolSample[2].size() << std::endl;
    intervalEdcaHolSample.clear();
}

void
MeasureIntervalThroughputHolDelay()
{
    Ns3AiMsgInterfaceImpl<Env, Act>* msgInterface =
        Ns3AiMsgInterface::Get()->GetInterface<Env, Act>();
    // std::cout << "\nInterval T " << Simulator::Now().GetSeconds() << std::endl;
    std::map<uint32_t, std::tuple<double, double, double, double>> nodeDelays;
    for (auto it : intervalEdcaHolSample)
    {
        uint32_t srcNodeId = it.first;
        double sum1 = 0;
        double sum2 = 0;
        double sum3 = 0;
        double sum4 = 0;

        for (double i = 0; i < it.second.size(); i += 4)
        {
            sum1 += it.second[i];
            sum2 += it.second[i + 1];
            sum3 += it.second[i + 2];
            sum4 += it.second[i + 3];
        }

        std::get<0>(nodeDelays[srcNodeId]) = sum1 / (it.second.size() / 4); // Avg HoL Delay
        std::get<1>(nodeDelays[srcNodeId]) = sum2 / (it.second.size() / 4); // Avg Queuing Delay
        std::get<2>(nodeDelays[srcNodeId]) = sum3 / (it.second.size() / 4); // Avg Access Delay
        std::get<3>(nodeDelays[srcNodeId]) = sum4 / (it.second.size() / 4); // Avg Tx Delay
    }

    Ptr<TgaxResidentialPropagationLossModel> propModel =
        CreateObject<TgaxResidentialPropagationLossModel>();
    GetRxPower(propModel);

    msgInterface->CppSendBegin();
    for (size_t i = 0; i < wifiNodes.GetN(); i++)
    {
        uint32_t txNodeId = wifiNodes.Get(i)->GetId();
        auto& env_struct = msgInterface->GetCpp2PyVector()->at(txNodeId);
        env_struct.txNode = txNodeId;
        env_struct.mcs = nodeMcs[txNodeId];
        env_struct.holDelay = std::get<0>(nodeDelays[txNodeId]);
        if (txNodeId >= N_BSS) // STAs
        {
            env_struct.throughput = (intervalBytesReceived.find(txNodeId)->second * 8) /
                                    static_cast<double>(Seconds(1).GetMicroSeconds());
        }
        else
        {
            // Only count for UL traffic
            env_struct.throughput = 0;
        }
        std::cout << "CPP send: txnode " << txNodeId << " tpt " << env_struct.throughput
                  << std::endl;
        for (auto rxNodePower : nodeRxPower[txNodeId])
        {
            NS_ASSERT(rxNodePower.first % N_BSS == 0); // only record rx node in first BSS
            if (rxNodePower.first == txNodeId)
            {
                env_struct.rxPower[rxNodePower.first / N_BSS] = 0;
            }
            else
            {
                env_struct.rxPower[rxNodePower.first / N_BSS] = rxNodePower.second;
            }
        }
    }
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    double nextCca = msgInterface->GetPy2CppVector()->at(0).newCcaSensitivity;
    msgInterface->CppRecvEnd();

    std::cout << "At " << Simulator::Now().GetMilliSeconds() << "ms:" << std::endl;

    // Change CCA of nodes in first BSS
    for (uint32_t i = 0; i < wifiNodes.GetN(); i += N_BSS)
    {
        uint32_t nodeId = wifiNodes.Get(i)->GetId();
        Ptr<NetDevice> dev = wifiNodes.Get(i)->GetDevice(0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        Ptr<WifiPhy> wifi_phy = wifi_dev->GetPhy();
        Ssid ssid = wifi_dev->GetMac()->GetSsid();
        // Only change CCA for nodes in BSS-0
        NS_ASSERT(ssid.IsEqual(Ssid("BSS-0")));
        double currentCca = wifi_phy->GetCcaSensitivityThreshold();

        Ptr<ThresholdPreambleDetectionModel> preambleCaptureModel =
            CreateObject<ThresholdPreambleDetectionModel>();
        preambleCaptureModel->SetAttribute("MinimumRssi", DoubleValue(nextCca));
        wifi_phy->SetCcaSensitivityThreshold(nextCca);
        wifi_phy->SetPreambleDetectionModel(preambleCaptureModel);
        std::cout << "-- " << ssid << " Node " << nodeId << " current CCA " << currentCca
                  << " next CCA " << nextCca << std::endl;
    }

    Simulator::ScheduleNow(&RestartIntervalThroughputHolDelay);
    Simulator::Schedule(Seconds(1), &MeasureIntervalThroughputHolDelay);

    //    // Set position for Nodes
    //    for (uint32_t i = 0; i < wifiNodes.GetN(); i++)
    //    {
    //        double x = 0;
    //        double y = 0;
    //
    //        x = randomX->GetValue() + ((i % apNodeCount) * boxSize);
    //        y = randomY->GetValue();
    //
    //        Vector l1(x, y, 1.5);
    //        Ptr<Object> object = wifiNodes.Get(i);
    //        Ptr<MobilityModel> model = object->GetObject<MobilityModel>();
    //        model->SetPosition(l1);
    //
    //        std::cout << "Node" << wifiNodes.Get(i)->GetId() << " " << x << "," << y << std::endl;
    //        // std::cout << "Points intersect how many walls? " <<
    //        // building->WallInLOS(l1, l2)
    //        //           << std::endl;
    //    }
    //    PrintPythonPlotCSV("box.csv");
}

// // std::map<uint32_t, std::vector<>>;

// /**
//  * PHY Rx trace.
//  *
//  * \param context The context.
//  * \param p The packet.
//  * \param power The Rx power.
//  */
// void
// PhyRxTrace(std::string context, Ptr<const Packet> p, RxPowerWattPerChannelBand power)
// {
//     for (auto bandPower : power)
//     {
//         std::cout << "PHY-RX-START time=" << Simulator::Now()
//                   << " node=" << ContextToNodeId(context) << " size=" << p->GetSize()
//                   << " Band Start " << bandPower.first.first << " Band end "
//                   << bandPower.first.second << " rxPower: " << bandPower.second << std::endl;
//     }
// }

std::vector<uint64_t> pid_r_packets;

/**
 * Trace a packet reception.
 *
 * \param context The context.
 * \param p The packet.
 * \param channelFreqMhz The channel frequqncy.
 * \param txVector The TX vector.
 * \param aMpdu The AMPDU.
 * \param signalNoise The signal and noise dBm.
 * \param staId The STA ID.
 */
void
TracePacketReception(std::string context,
                     Ptr<const Packet> p,
                     uint16_t channelFreqMhz,
                     WifiTxVector txVector,
                     MpduInfo aMpdu,
                     SignalNoiseDbm signalNoise,
                     uint16_t staId)
{
    Ptr<Packet> packet = p->Copy();
    auto it = std::find(pid_r_packets.begin(), pid_r_packets.end(), packet->GetUid());

    if (it == pid_r_packets.end())
    {
        pid_r_packets.push_back(packet->GetUid());
    }
    else
    {
        return;
    }
    if (txVector.IsAggregation())
    {
        AmpduSubframeHeader subHdr;
        uint32_t extractedLength;
        packet->RemoveHeader(subHdr);
        extractedLength = subHdr.GetLength();
        packet = packet->CreateFragment(0, static_cast<uint32_t>(extractedLength));
    }
    WifiMacHeader hdr;
    packet->PeekHeader(hdr);

    if (hdr.GetAddr1() != ContextToMac(context))
    {
        return;
    }
    // std::cout << "Sender " << hdr.GetAddr2() << " Destination (packet) " << hdr.GetAddr1()
    //           << " t_Destination:" << ContextToMac(context) << std::endl;

    if (hdr.IsData()) // ignore non-data frames
    {
        // std::cout << "Packet size: " << packet->GetSize() << std::endl;
        // std::cout << "SNIFFERRX" << std::endl;

        IncrementCounter(packetsReceived, hdr.GetAddr2());
        IncrementCounter(bytesReceived, hdr.GetAddr2(), packet->GetSize());
        intervalBytesReceived[MacAddressToNodeId(hdr.GetAddr2())] += packet->GetSize();
        // IncrementCounter(intervalBytesReceived, hdr.GetAddr2(), packet->GetSize());
        auto itTimeFirstReceived = timeFirstReceived.find(hdr.GetAddr2());
        if (itTimeFirstReceived == timeFirstReceived.end())
        {
            // std::cout << "First Time see addy: " << hdr.GetAddr2() << " at "
            //           << Simulator::Now().GetSeconds() << std::endl;
            timeFirstReceived.insert(std::make_pair(hdr.GetAddr2(), Simulator::Now()));
        }
        auto itTimeLastReceived = timeLastReceived.find(hdr.GetAddr2());
        if (itTimeLastReceived != timeLastReceived.end())
        {
            // std::cout << "Last Time see addy: " << hdr.GetAddr2() << " at "
            //           << Simulator::Now().GetSeconds() << std::endl;
            itTimeLastReceived->second = Simulator::Now();
        }
        else
        {
            timeLastReceived.insert(std::make_pair(hdr.GetAddr2(), Simulator::Now()));
        }
    }
}

int drops = 0;
int receives = 0;
// rxNode, packet, reason, time
std::map<uint32_t, std::map<uint64_t, std::vector<std::pair<WifiPhyRxfailureReason, Time>>>>
    nodeFailureCount;
std::map<WifiPhyRxfailureReason, int> typeFailCount;

void
PhyDrop(std::string context, Ptr<const Packet> p, WifiPhyRxfailureReason reas)
{
    Ptr<Packet> packet = p->Copy();

    WifiMacHeader hdr;
    packet->PeekHeader(hdr);

    if (hdr.GetAddr1() != ContextToMac(context))
    {
        return;
    }

    nodeFailureCount[ContextToNodeId(context)][p->GetUid()].push_back(
        std::make_pair(reas, Simulator::Now()));
    drops++;
    // if (hdr.HasData()) // ignore non-data frames
    // {
    //     if (packet->GetSize() >= pktSize)
    //     {

    // if (p->GetSize() >= pktSize)
    // {
    // std::cout << "PHYDROPP " << reas << " at " << Simulator::Now().GetSeconds() << " Packet "
    //           << p->GetUid() << std::endl;
    typeFailCount[reas] += 1;

    //     }
    // }

    // if (packet->GetSize() >= pktSize) // ignore non-data frames
    // {
    //     drops++;
    //     // std::cout << "PHYDROP" << std::endl;
    // }
    // if (reas == WifiPhyRxfailureReason(RXING))
    // {

    // }
    // nodeFailureCount[ContextToNodeId(context)][reas] += 1;

    // std::cout << "PHYDROP Reason: " << reas << std::endl;
}

// /**
//  * \ingroup wifi
//  * Enumeration of the possible reception failure reasons.
//  */
// enum WifiPhyRxfailureReason
// {
//     UNKNOWN = 0,
//     UNSUPPORTED_SETTINGS,
//     CHANNEL_SWITCHING,
//     RXING,
//     TXING,
//     SLEEPING,
//     POWERED_OFF,
//     TRUNCATED_TX,
//     BUSY_DECODING_PREAMBLE,
//     PREAMBLE_DETECT_FAILURE,
//     RECEPTION_ABORTED_BY_TX,
//     L_SIG_FAILURE,
//     HT_SIG_FAILURE,
//     SIG_A_FAILURE,
//     SIG_B_FAILURE,
//     U_SIG_FAILURE,
//     EHT_SIG_FAILURE,
//     PREAMBLE_DETECTION_PACKET_SWITCH,
//     FRAME_CAPTURE_PACKET_SWITCH,
//     OBSS_PD_CCA_RESET,
//     HE_TB_PPDU_TOO_LATE,
//     FILTERED,
//     DMG_HEADER_FAILURE,
//     DMG_ALLOCATION_ENDED
// };

std::map<uint32_t, std::map<uint64_t, Time>> nodeSuccessCount;

void
PhyEnd(std::string context, Ptr<const Packet> p)
{
    // std::cout << "PHYRx End packet" << p->GetUid() << std::endl;
    Ptr<Packet> packet = p->Copy();

    WifiMacHeader hdr;
    packet->PeekHeader(hdr);

    if (hdr.GetAddr1() != ContextToMac(context))
    {
        return;
    }
    nodeSuccessCount[ContextToNodeId(context)][p->GetUid()] = Simulator::Now();
    if (packet->GetSize() >= pktSize) // ignore non-data frames
    {
        receives++;
        // std::cout << "PHYDROP" << std::endl;
    }
}

void
ChangeCcaSensitivity(double stepSize, double intervalLength)
{
    for (uint32_t i = 0; i < wifiNodes.GetN(); i++)
    {
        Ptr<NetDevice> dev = wifiNodes.Get(i)->GetDevice(0);
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        Ptr<WifiPhy> wifi_phy = wifi_dev->GetPhy();
        double currentCca = wifi_phy->GetCcaSensitivityThreshold();
        // std::cout << "Current Cca: " << currentCca << " next Cca: " << currentCca + stepSize
        //           << std::endl;
        Ptr<ThresholdPreambleDetectionModel> preambleCaptureModel =
            CreateObject<ThresholdPreambleDetectionModel>();
        preambleCaptureModel->SetAttribute("MinimumRssi", DoubleValue(currentCca + stepSize));
        wifi_phy->SetCcaSensitivityThreshold(currentCca + stepSize);
        wifi_phy->SetPreambleDetectionModel(preambleCaptureModel);
    }
    Simulator::Schedule(Seconds(intervalLength), &ChangeCcaSensitivity, stepSize, intervalLength);
}

void
AssociatedSta(uint16_t aid, Mac48Address addy /* addr */)
{
    associatedStas++;
    std::cout << "Node " << MacAddressToNodeId(addy)
              << " associated at T=" << Simulator::Now().GetSeconds() << std::endl;
}

void
DeAssociatedSta(uint16_t aid, Mac48Address /* addr */)
{
    deassociatedStas++;
}

std::string
AddressToString(const Address& addr)
{
    std::stringstream addressStr;
    addressStr << InetSocketAddress::ConvertFrom(addr).GetIpv4() << ":"
               << InetSocketAddress::ConvertFrom(addr).GetPort();
    return addressStr.str();
}

void
FragmentTx(Ptr<const Packet> fragment,
           const Address& from,
           const Address& to,
           const SeqTsSizeFragHeader& header)
{
    NS_LOG_INFO("Sent fragment " << header.GetFragSeq() << "/" << header.GetFrags()
                                 << " of burst seq=" << header.GetSeq()
                                 << " of header.GetSize ()=" << header.GetSize()
                                 << " (fragment->GetSize ()=" << fragment->GetSize()
                                 << ") bytes from " << AddressToString(from) << " to "
                                 << AddressToString(to) << " at " << header.GetTs().As(Time::S));
    Ptr<const Packet> p = fragment;

    auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    if (mapIt == m_inFlightPacketMap.end())
    {
        // std::cout << "No packet with UID " << p->GetUid() << " is currently in queue" <<
        // std::endl;
        return;
    }

    auto listIt =
        std::find_if(mapIt->second.begin(),
                     mapIt->second.end(),
                     [&p](const InFlightPacketInfo& info) { return info.m_ptrToPacket == p; });

    if (listIt == mapIt->second.end())
    {
        // std::cout << "Forwarding up a packet that has not been enqueued?" << std::endl;
        return;
    }

    InFlightPacketInfo info;
    info.m_srcAddress = listIt->m_srcAddress;
    info.m_dstAddress = listIt->m_dstAddress;
    info.m_ptrToPacket = listIt->m_ptrToPacket;
    info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    info.m_edcaDequeueTime = listIt->m_edcaDequeueTime;
    info.appTypeTxTime = Simulator::Now();

    mapIt->second.erase(listIt);
    mapIt->second.insert(mapIt->second.end(), info);
}

void
FragmentRx(Ptr<const Packet> fragment,
           const Address& from,
           const Address& to,
           const SeqTsSizeFragHeader& header)
{
    NS_LOG_INFO("Received fragment "
                << header.GetFragSeq() << "/" << header.GetFrags() << " of burst seq="
                << header.GetSeq() << " of header.GetSize ()=" << header.GetSize()
                << " (fragment->GetSize ()=" << fragment->GetSize() << ") bytes from "
                << AddressToString(from) << " to " << AddressToString(to) << " at "
                << header.GetTs().As(Time::S));
    Ptr<const Packet> p = fragment;

    auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    if (mapIt == m_inFlightPacketMap.end())
    {
        std::cout << "No packet with UID " << p->GetUid() << " is currently in queue" << std::endl;
        return;
    }
    // mapIt->second.;
    auto listIt = std::find_if(mapIt->second.begin(),
                               mapIt->second.end(),
                               [&p](const InFlightPacketInfo& info) {
                                   return info.m_ptrToPacket->GetUid() == p->GetUid();
                               });

    if (listIt == mapIt->second.end())
    {
        std::cout << "Forwarding up a packet that has not been enqueued?" << std::endl;
        return;
    }
    InFlightPacketInfo info;
    info.m_srcAddress = listIt->m_srcAddress;
    info.m_dstAddress = listIt->m_dstAddress;
    info.m_ptrToPacket = listIt->m_ptrToPacket;
    info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    info.appTypeTxTime = listIt->appTypeTxTime;
    info.m_phyTxTime = listIt->m_phyTxTime;
    info.m_L2RxTime = listIt->m_L2RxTime;
    info.appTypeRxTime = Simulator::Now();
    // if (p->GetSize() >= pktSize) // ignore non-data frames
    // {
    appTxrec++;
    // }
    // else
    // {
    //     std::cout << "Size " << p->GetSize() << std::endl;
    // }
    // std::cout << "APRX" << std::endl;

    mapIt->second.erase(listIt);
    mapIt->second.insert(mapIt->second.end(), info);
    // appTxrec++;
}

void
BurstTx(Ptr<const Packet> burst,
        const Address& from,
        const Address& to,
        const SeqTsSizeFragHeader& header)
{
    NS_LOG_INFO("Sent burst seq=" << header.GetSeq() << " of header.GetSize ()=" << header.GetSize()
                                  << " (burst->GetSize ()=" << burst->GetSize() << ") bytes from "
                                  << AddressToString(from) << " to " << AddressToString(to)
                                  << " at " << header.GetTs().As(Time::S));
    // std::cout << "APPTX" << std::endl;
}

void
BurstRx(Ptr<const Packet> burst,
        const Address& from,
        const Address& to,
        const SeqTsSizeFragHeader& header)
{
    NS_LOG_INFO("Received burst seq="
                << header.GetSeq() << " of header.GetSize ()=" << header.GetSize()
                << " (burst->GetSize ()=" << burst->GetSize() << ") bytes from "
                << AddressToString(from) << " to " << AddressToString(to) << " at "
                << header.GetTs().As(Time::S));
    // std::cout << "APPRX" << std::endl;
    // appTxrec++;
    // appTxrec++;
    // std::cout << "APRX" << std::endl;

    // Ptr<const Packet> p = burst;

    // auto mapIt = m_inFlightPacketMap.find(p->GetUid());
    // if (mapIt == m_inFlightPacketMap.end())
    // {
    //     std::cout << "No packet with UID " << p->GetUid() << " is currently in queue" <<
    //     std::endl; return;
    // }
    // // mapIt->second.;
    // auto listIt = std::find_if(mapIt->second.begin(),
    //                            mapIt->second.end(),
    //                            [&p](const InFlightPacketInfo& info) {
    //                                return info.m_ptrToPacket->GetUid() == p->GetUid();
    //                            });

    // if (listIt == mapIt->second.end())
    // {
    //     std::cout << "Forwarding up a packet that has not been enqueued?" << std::endl;
    //     return;
    // }
    // InFlightPacketInfo info;
    // info.m_srcAddress = listIt->m_srcAddress;
    // info.m_dstAddress = listIt->m_dstAddress;
    // info.m_ptrToPacket = listIt->m_ptrToPacket;
    // info.m_edcaEnqueueTime = listIt->m_edcaEnqueueTime;
    // info.appTypeTxTime = listIt->appTypeTxTime;
    // info.m_phyTxTime = listIt->m_phyTxTime;
    // info.m_L2RxTime = listIt->m_L2RxTime;
    // info.appTypeRxTime = Simulator::Now();

    // // appTxrec++;
    // // std::cout << "APRX" << std::endl;

    // mapIt->second.erase(listIt);
    // mapIt->second.insert(mapIt->second.end(), info);
}

// Works by reading the full string into a file to later be parsed in main
std::map<int, std::string>
readConfigFile(const std::string& filename)
{
    std::map<int, std::string> config;

    std::ifstream configFile(filename);
    if (!configFile)
    {
        std::cerr << "Error opening configuration file: " << filename << std::endl;
        return config;
    }

    std::string line;
    std::string ready;
    while (std::getline(configFile, line))
    {
        if (!line.find('#'))
        {
            continue;
        }
        size_t delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos)
        {
            // std::cout << line.substr(0, delimiterPos) << std::endl;
            int node = std::stoi(line.substr(0, delimiterPos));
            std::string configLine = line.substr(delimiterPos + 1);
            config[node] = configLine;
        }
    }

    configFile.close();
    return config;
}

void
CreateTrafficConfigurationTemplate(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename.c_str(), std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }
    outFile << "#NodeId:TrafficType,CcaSensitivity,TxPower,ChannelWidth,ChannelNumber\n"
            << std::endl;
    for (uint32_t i = 0; i < apNodes.GetN(); i++)
    {
        outFile << "#BSS-" << i << "\n#AP\n"
                << i << ":"
                << "\n#STA" << std::endl;
        for (uint32_t x = 0; x < staNodes.GetN(); x += apNodes.GetN())
        {
            outFile << staNodes.Get(x + i)->GetId() << ":" << std::endl;
        }
        outFile << std::endl;
    }
    outFile.close();
}

/**
 * Contention window trace.
 *
 * \param context The context.
 * \param cw The contention window.
 */
void
CwTrace(std::string context, uint32_t cw, uint8_t /* linkId */)
{
    // std::cout << Simulator::Now().GetSeconds() << " " << ContextToNodeId(context) << " " <<
    // cw
    //           << std::endl;
    nodeCw[ContextToNodeId(context)].push_back(cw);
}

/**
 * Backoff trace.
 *
 * \param context The context.
 * \param newVal The backoff value.
 */
void
BackoffTrace(std::string context, uint32_t newVal, uint8_t /* linkId */)
{
    // std::cout << Simulator::Now().GetSeconds() << " " << ContextToNodeId(context) << " " <<
    // newVal
    //           << std::endl;
    nodeBackoff[ContextToNodeId(context)].push_back(newVal);
}

struct overlappingPackets
{
    uint32_t nodeID{0};
    uint32_t ifNodeID{0};
    uint32_t rxNodeID{0};
    uint64_t packet{0};
    uint64_t ifPacket{0};
    Time startTime{0};
    Time endTime{0};
    Time ifStartTime{0};
    Time ifEndTime{0};
    Time phyDropTime{0};
    WifiPhyRxfailureReason reason;
    std::string sync{""};

    bool operator==(const overlappingPackets& other)
    {
        return (nodeID == other.nodeID) && (ifNodeID == other.ifNodeID) &&
               (rxNodeID == other.rxNodeID) && (packet == other.packet) &&
               (ifPacket == other.ifPacket) && (reason == other.reason);
    }
};

std::vector<overlappingPackets> packetOverlapList;
std::vector<overlappingPackets> packetOverlapSuccessList;
std::unordered_map<WifiPhyRxfailureReason, int> typeOverlapCount;
int totalSimulTx = 0;

void
createPacketPairs()
{
    std::vector<overlappingPackets> packetPossibleOverlapList;
    // packetPossibleOverlapList.reserve(nodePacketTxTime.size() * average_packets_per_node);

    for (const auto& nodePacketTime : nodePacketTxTime)
    {
        uint32_t nodeID = nodePacketTime.first;

        for (const auto& packetTime : nodePacketTime.second)
        {
            uint32_t packetID = packetTime.first;

            const auto& txEndTimes = nodePacketTxEndTime[nodeID][packetID];
            const size_t overlapCount = std::min(packetTime.second.size(), txEndTimes.size());

            for (size_t i = 0; i < overlapCount; i++)
            {
                overlappingPackets oPackets;
                oPackets.nodeID = nodeID;
                oPackets.packet = packetID;
                oPackets.startTime = packetTime.second[i];
                oPackets.endTime = txEndTimes[i];
                packetPossibleOverlapList.push_back(oPackets);
            }
        }
    }

    std::vector<overlappingPackets> packetOverlapList2;
    packetOverlapList2.reserve(packetPossibleOverlapList.size());

    for (const auto& packet : packetPossibleOverlapList)
    {
        for (const auto& ifPacket : packetPossibleOverlapList)
        {
            if (packet.endTime >= ifPacket.startTime && packet.startTime <= ifPacket.endTime)
            {
                overlappingPackets oPackets;
                oPackets = packet;
                oPackets.ifNodeID = ifPacket.nodeID;
                oPackets.ifPacket = ifPacket.packet;
                oPackets.ifStartTime = ifPacket.startTime;
                oPackets.ifEndTime = ifPacket.endTime;
                oPackets.sync = (std::abs(packet.startTime.GetMicroSeconds() -
                                          ifPacket.startTime.GetMicroSeconds()) < 4)
                                    ? "synchronously"
                                    : "asynchronously";
                packetOverlapList2.push_back(oPackets);
            }
        }
    }

    for (const auto& packet : packetOverlapList2)
    {
        for (const auto& packetReasTime : nodeFailureCount)
        {
            auto iter = packetReasTime.second.find(packet.packet);
            if (iter != packetReasTime.second.end())
            {
                for (const auto& reasTime : iter->second)
                {
                    if (packet.startTime < reasTime.second && packet.endTime > reasTime.second)
                    {
                        // std::cout << "Time difference between end of tx and rx drop"
                        //           << packet.endTime.GetMicroSeconds() -
                        //                  reasTime.second.GetMicroSeconds()
                        //           << std::endl;
                        overlappingPackets oPackets;
                        oPackets = packet;
                        oPackets.rxNodeID = packetReasTime.first;
                        oPackets.reason = reasTime.first;
                        oPackets.phyDropTime = reasTime.second;
                        // Skip adding overlap if source node is the same as interfering node or the
                        // interfering packet is the same as the tx packet
                        if (!((packet.nodeID == packet.ifNodeID) ||
                              (packet.packet == packet.ifPacket)))
                        {
                            // Skip adding if the drop reason is TXING and the ifNode is not the
                            // rxNode (how could you be txing if the interferer is not the node that
                            // noticed the txing? <-basically)
                            if (((oPackets.reason == WifiPhyRxfailureReason(TXING)) ||
                                 oPackets.reason ==
                                     WifiPhyRxfailureReason(RECEPTION_ABORTED_BY_TX)) &&
                                (packet.ifNodeID != oPackets.rxNodeID))
                            {
                                // std::cout << "drop txing" << std::endl;
                                continue;
                            }
                            // Skip adding if the drop reason is RXING or BUSY_DECODING_PREAMBLE and
                            // the interferer node is the receiving node
                            else if (((oPackets.reason == WifiPhyRxfailureReason(RXING)) ||
                                      (oPackets.reason ==
                                       WifiPhyRxfailureReason(BUSY_DECODING_PREAMBLE)) ||
                                      (oPackets.reason ==
                                       WifiPhyRxfailureReason(PREAMBLE_DETECT_FAILURE))) &&
                                     (packet.ifNodeID == oPackets.rxNodeID))
                            {
                                // std::cout << "dropped rxing" << std::endl;
                                continue;
                            }
                            // Skip adding overlapping packets that were received at the same time
                            // but got dropped due to RXING because it means a previous packet was
                            // already being decoded when this two additional signals colided due to
                            // the same backoff
                            else if (oPackets.reason == WifiPhyRxfailureReason(RXING) &&
                                     oPackets.sync == "synchronously")
                            {
                                // packetOverlapList.emplace_back(oPackets);
                                continue;
                            }
                            else
                            {
                                packetOverlapList.emplace_back(oPackets);
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
            }
        }
    }

    for (const auto& packetSTime : nodeSuccessCount)
    {
        for (const auto& packet : packetOverlapList2)
        {
            auto iter = packetSTime.second.find(packet.packet);
            if (iter != packetSTime.second.end())
            {
                if (!((packet.nodeID == packet.ifNodeID) || (packet.packet == packet.ifPacket)))
                {
                    // std::cout << "Packet " << iter->first
                    //           << " Time difference between end of tx and rx done "
                    //           << iter->second.GetNanoSeconds() - packet.endTime.GetNanoSeconds()
                    //           << " start time: " << packet.startTime << std::endl;
                    if (packet.ifEndTime.GetSeconds() != 0)
                    {
                        if (std::abs(iter->second.GetNanoSeconds() -
                                     packet.endTime.GetNanoSeconds()) <= 30)
                        {
                            // if ((duration + 10) - iter->second.GetSeconds() > 0.1)
                            // {
                            overlappingPackets oPackets;
                            oPackets = packet;
                            oPackets.rxNodeID = packetSTime.first;
                            oPackets.phyDropTime = iter->second;
                            // std::cout << "Node " << oPackets.nodeID << " Tx the packet "
                            //           << oPackets.packet << " and it overlapped " <<
                            //           oPackets.sync
                            //           << " with packet " << oPackets.ifPacket << " from Node
                            //           "
                            //           << oPackets.ifNodeID
                            //           << "\n but the first packet did not get dropped. It was
                            //           "
                            //              "succesfully received at T= "
                            //           << oPackets.phyDropTime.GetSeconds() << "\n"
                            //           << std::endl;
                            packetOverlapSuccessList.emplace_back(oPackets);
                            // }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Report Rate changed.
 *
 * \param oldVal Old value.
 * \param newVal New value.
 */
void
RateChange(std::string context, uint64_t oldVal, uint64_t newVal)
{
    nodeMcs[ContextToNodeId(context)] = dataRateToMcs[newVal];
    // std::cout << "Datarate: " << dataRateToMcs[newVal] << std::endl;
    // nodeMcs
}

std::unordered_map<uint64_t, int> bssNode;

int
main(int argc, char* argv[])
{
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(true);
    interface->SetHandleFinish(true);
    duration = 100;    ///< duration (in seconds)
    bool pcap = false; ///< Flag to enable/disable PCAP files generation
    uint32_t seedNumber = 1;
    std::string traceFolder =
        "src/vr-app/model/BurstGeneratorTraces/"; // example traces can be found here
    std::string traceFile = "ge_cities_20mbps_30fps.csv";

    std::string phyMode = "OfdmRate54Mbps"; ///< Constant PHY mode
    double frequency = 5;                   ///< The operating frequency band in GHz: 2.4, 5 or 6
    uint16_t channelWidths = 20;     ///< The constant channel width in MHz (only for 11n/ac/ax)
    uint16_t guardIntervalNs = 3200; ///< The guard interval in nanoseconds (800 or 400 for
                                     ///< 11n/ac, 800 or 1600 or 3200 for 11 ax)
    uint16_t pktInterval =
        1000; ///< The socket packet interval in microseconds (a higher value is needed to reach
    ///< saturation conditions as the channel bandwidth or the MCS increases)

    txPower = 16; ///< The transmit power of all nodes in dBm (or if --app=setup-done, custom
                  ///< txPowers)
    ccaSensitivity = -82;
    std::string standard("11ax"); ///< the 802.11 standard
    // Disable fragmentation and RTS/CTS
    int mcs = -1;

    networkSize = 0;

    Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
                       StringValue("22000"));
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("22000"));
    // Disable short retransmission failure (make retransmissions persistent)
    Config::SetDefault("ns3::WifiRemoteStationManager::MaxSlrc",
                       UintegerValue(std::numeric_limits<uint32_t>::max()));
    Config::SetDefault("ns3::WifiRemoteStationManager::MaxSsrc",
                       UintegerValue(std::numeric_limits<uint32_t>::max()));
    // Set maximum queue size to the largest value and set maximum queue delay to be larger than
    // the simulation time
    Config::SetDefault("ns3::WifiMacQueue::MaxSize",
                       QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS,
                                                100))); // TODO: set to a smaller value. 100?
    Config::SetDefault("ns3::WifiMacQueue::MaxDelay", TimeValue(Seconds(20 * duration)));
    bool calculateStats = false;
    int ring = 0;
    bool autoMCS = false;
    std::string configFileName = "foo.config.txt";
    CommandLine cmd(__FILE__);
    cmd.AddValue("pktSize", "The packet size in bytes", pktSize);
    cmd.AddValue("rng", "The seed run number", seedNumber);
    cmd.AddValue("app",
                 "The type of application to set. (constant,bursty,bursty-trace,setup-setup-done)",
                 appType);
    cmd.AddValue("prop", "The propagation loss model", propagationModel);
    cmd.AddValue("ring", "Set ring topology or not", ring);
    cmd.AddValue("ccaSensitivity", "The cca sensitivity (-82dBm)", ccaSensitivity);
    cmd.AddValue("duration", "Time duration for each trial in seconds", duration);
    cmd.AddValue("pcap", "Enable/disable PCAP tracing", pcap);
    cmd.AddValue("autoMCS", "Enable/disable automatic choice of MCS", autoMCS);
    cmd.AddValue("traceFolder", "The folder containing the trace.", traceFolder);
    cmd.AddValue("traceFile", "The trace file name.", traceFile);
    cmd.AddValue("networkSize", "Number of stations per bss", networkSize);
    cmd.AddValue("standard", "Set the standard (11a, 11b, 11g, 11n, 11ac, 11ax)", standard);
    cmd.AddValue("apNodes", "Number of APs", apNodeCount); // use 4
    cmd.AddValue("phyMode", "Set the constant PHY mode string used to transmit frames", phyMode);
    cmd.AddValue("frequency", "Set the operating frequency band in GHz: 2.4, 5 or 6", frequency);
    cmd.AddValue("overlapStats",
                 "Enable the calculation of overlapping packets and their source",
                 calculateStats);
    cmd.AddValue("channelWidth",
                 "Set the constant channel width in MHz (only for 11n/ac/ax)",
                 channelWidths);
    cmd.AddValue("gi",
                 "Set the the guard interval in nanoseconds (800 or 400 for 11n/ac, 800 or 1600 or "
                 "3200 for 11 ax)",
                 guardIntervalNs);
    cmd.AddValue("maxMpdus",
                 "Set the maximum number of MPDUs in A-MPDUs (0 to disable MPDU aggregation)",
                 maxMpdus);
    cmd.AddValue("distance", "Set the distance in meters between the AP and the STAs", distance);
    cmd.AddValue("txPower", "Set the transmit power of all nodes in dBm", txPower);
    cmd.AddValue("pktInterval", "Set the socket packet interval in microseconds", pktInterval);
    cmd.AddValue("boxsize", "Set the size of the box in meters", boxSize);
    cmd.AddValue("drl", "Enable the use of DRL for setting ccaSensitivity", drlCca);
    cmd.AddValue("configFile", "Configuration file of Multi-BSS example", configFileName);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seedNumber);
    RngSeedManager::SetRun(seedNumber);

    for (int i = 0; i < 12; i++)
    {
        // std::cout << "Datarate: " << HePhy::GetDataRate(i, channelWidths, guardIntervalNs, 1)
        //           << std::endl;
        dataRateToMcs[HePhy::GetDataRate(i, channelWidths, NanoSeconds(guardIntervalNs), 1)] = i;
    }

    if (phyMode != "OfdmRate54Mbps")
    {
        mcs = std::stoi(phyMode.substr(phyMode.find("s") + 1));
    }
    if (networkSize < 3)
    {
        pktInterval = 100;
    }
    // LogComponentEnable("StaWifiMac", LOG_LEVEL_ALL);
    // LogComponentEnable("WifiAssocManager", LOG_LEVEL_ALL);
    // LogComponentEnable("ApWifiMac", LOG_LEVEL_ALL);

    // LogComponentEnable("PhyEntity", LOG_LEVEL_ALL);
    int channelWidth = channelWidths;
    int gi = guardIntervalNs;
    apNodes.Create(apNodeCount);
    staNodes.Create(apNodeCount * networkSize);
    if (appType == "setup")
    {
        CreateTrafficConfigurationTemplate(configFileName);
        std::cout << "Created config files for traffic based on current topology. Specify each "
                     "traffic type and run --app=setup-done. Traffic "
                     "types(none,constant,bursty,bursty-trace-TRACELOCATION)"
                  << std::endl;
        return 0;
    }
    else if (appType == "setup-done")
    {
        configuration = readConfigFile(configFileName);
    }

    // std::cout << "Traffic type for STA1: " << configuration[2] << std::endl;

    WifiStandard wifiStandard;
    if (standard == "11a")
    {
        wifiStandard = WIFI_STANDARD_80211a;
        frequency = 5;
        channelWidth = 20;
    }
    else if (standard == "11b")
    {
        wifiStandard = WIFI_STANDARD_80211b;
        frequency = 2.4;
        channelWidth = 22;
    }
    else if (standard == "11g")
    {
        wifiStandard = WIFI_STANDARD_80211g;
        frequency = 2.4;
        channelWidth = 20;
    }
    else if (standard == "11n")
    {
        if (frequency == 2.4)
        {
            wifiStandard = WIFI_STANDARD_80211n;
        }
        else if (frequency == 5)
        {
            wifiStandard = WIFI_STANDARD_80211n;
        }
        else
        {
            NS_FATAL_ERROR("Unsupported frequency band " << frequency << " GHz for standard "
                                                         << standard);
        }
    }
    else if (standard == "11ac")
    {
        wifiStandard = WIFI_STANDARD_80211ac;
        frequency = 5;
    }
    else if (standard == "11ax")
    {
        if (frequency == 2.4)
        {
            wifiStandard = WIFI_STANDARD_80211ax;
        }
        else if (frequency == 5)
        {
            wifiStandard = WIFI_STANDARD_80211ax;
        }
        else if (frequency == 6)
        {
            wifiStandard = WIFI_STANDARD_80211ax;
        }
        else
        {
            NS_FATAL_ERROR("Unsupported frequency band " << frequency << " GHz for standard "
                                                         << standard);
        }
    }
    else
    {
        NS_FATAL_ERROR("Unsupported standard: " << standard);
    }

    if (appType != "setup-done")
    {
        std::string channelStr = "{0, " + std::to_string(channelWidth) + ", BAND_" +
                                 (frequency == 2.4 ? "2_4" : (frequency == 5 ? "5" : "6")) +
                                 "GHZ, 0}";
        Config::SetDefault("ns3::WifiPhy::ChannelSettings", StringValue(channelStr));
    }

    YansWifiChannelHelper wifiChannel;

    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    if (frequency == 6)
    {
        if (propagationModel == "log")
        {
            // Reference Loss for Friss at 1 m with 6.0 GHz
            wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                           "Exponent",
                                           DoubleValue(2.0),
                                           "ReferenceDistance",
                                           DoubleValue(1.0),
                                           "ReferenceLoss",
                                           DoubleValue(49.013));
        }
        else if (propagationModel == "tgax")
        {
            wifiChannel.AddPropagationLoss("ns3::TgaxResidentialPropagationLossModel",
                                           "Frequency",
                                           DoubleValue(6e9),
                                           "ShadowSigma",
                                           DoubleValue(5.0));
        }
        else if (propagationModel == "fixed")
        {
            wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(-71));
        }
    }
    else if (frequency == 5)
    {
        // Reference Loss for Friss at 1 m with 5.15 GHz
        if (propagationModel == "log")
        {
            wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                           "Exponent",
                                           DoubleValue(3.0),
                                           "ReferenceDistance",
                                           DoubleValue(1.0),
                                           "ReferenceLoss",
                                           DoubleValue(50));
        }
        else if (propagationModel == "tgax")
        {
            wifiChannel.AddPropagationLoss("ns3::TgaxResidentialPropagationLossModel",
                                           "Frequency",
                                           DoubleValue(5e9),
                                           "ShadowSigma",
                                           DoubleValue(5.0));
        }
        else if (propagationModel == "fixed")
        {
            wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(-71));
        }
    }
    else
    {
        // Reference Loss for Friss at 1 m with 2.4 GHz
        if (propagationModel == "log")
        {
            wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                           "Exponent",
                                           DoubleValue(2.0),
                                           "ReferenceDistance",
                                           DoubleValue(1.0),
                                           "ReferenceLoss",
                                           DoubleValue(40.046));
        }
        else if (propagationModel == "tgax")
        {
            wifiChannel.AddPropagationLoss("ns3::TgaxResidentialPropagationLossModel",
                                           "Frequency",
                                           DoubleValue(2.4e9),
                                           "ShadowSigma",
                                           DoubleValue(5.0));
        }
        else if (propagationModel == "fixed")
        {
            wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(-71));
        }
    }

    WifiHelper wifi;
    wifi.SetStandard(wifiStandard);
    if (!autoMCS)
    {
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode",
                                     StringValue(phyMode),
                                     "ControlMode",
                                     StringValue("OfdmRate54Mbps"));
    }
    else
    {
        Config::SetDefault("ns3::AutoMcsWifiManager::autoMCS", BooleanValue(autoMCS));
        wifi.SetRemoteStationManager("ns3::AutoMcsWifiManager");
    }
    YansWifiPhyHelper phy;
    phy.SetErrorRateModel("ns3::NistErrorRateModel");

    phy.SetChannel(wifiChannel.Create());
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    // // phy.Set ("RxSensitivity", DoubleValue (-91));
    // phy.DisablePreambleDetectionModel();
    // // phy.SetCcaSensitivityThreshold(m_CcaSensitivityDbm);
    // phy.Set("CcaEdThreshold", DoubleValue(ccaSensitivity + 40));

    if (appType != "setup-done")
    {
        phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel",
                                      "MinimumRssi",
                                      DoubleValue(ccaSensitivity));
        phy.Set("CcaSensitivity", DoubleValue(ccaSensitivity));
        // phy.Set("CcaEdThreshold", DoubleValue(ccaSensitivity + 20));
        phy.Set("TxPowerStart", DoubleValue(txPower));
        phy.Set("TxPowerEnd", DoubleValue(txPower));
    }
    uint64_t beaconInterval = 100 * 1024;

    // phy.Set("TxPowerStart", DoubleValue(txPower));
    // phy.Set("TxPowerEnd", DoubleValue(txPower));
    WifiMacHelper mac;
    for (int i = 0; i < apNodeCount; ++i)
    {
        if (appType == "setup-done")
        {
            std::string configString = configuration[apNodes.Get(i)->GetId()];
            std::vector<std::string> configValues = csv_split(configString, ',');
            double m_ccaSensitivity = std::stoi(configValues[1]);
            double m_txPower = std::stoi(configValues[2]);
            //            double m_chWidth = std::stoi(configValues[3]);
            //            double m_chNumber = std::stoi(configValues[4]);

            phy.Set("CcaSensitivity", DoubleValue(m_ccaSensitivity));
            phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel",
                                          "MinimumRssi",
                                          DoubleValue(m_ccaSensitivity));
            phy.Set("TxPowerStart", DoubleValue(m_txPower));
            phy.Set("TxPowerEnd", DoubleValue(m_txPower));
            std::string chStr = "{" + configValues[4] + "," + configValues[3] + ", BAND_5GHZ, 0}";
            phy.Set("ChannelSettings", StringValue(chStr));
            // phy.Set("ChannelWidth", UintegerValue(m_chWidth));
            // phy.Set("ChannelNumber", UintegerValue(m_chNumber));
            // phy.Set("Frequency", UintegerValue(frequency));
            // for(auto it :configValues ){
            //     std::cout << "AP Values " << it << std::endl;
            // }

            // double m_ccaSensitivity = std::stoi(configValues[0]);

            // size_t pos = configString.find(',');
            // configString = configString.substr(pos + 1);
            // size_t pos2 = configString.find(',');
            // std::cout << "CCaSensitivity: " << configString.substr(0, pos2) << std::endl;
            // double m_ccaSensitivity = std::stoi(configString.substr(0, pos2));
            // std::cout << "apTxPower: " << configString.substr(pos2 + 1) << std::endl;
            // double m_txPower = std::stoi(configString.substr(pos2 + 1));
            // phy.Set("CcaSensitivity", DoubleValue(m_ccaSensitivity));
            // phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel",
            //                               "MinimumRssi",
            //                               DoubleValue(m_ccaSensitivity));
            // phy.Set("TxPowerStart", DoubleValue(m_txPower));
            // phy.Set("TxPowerEnd", DoubleValue(m_txPower));
        }
        std::string ssi = "BSS-" + std::to_string(i);
        Ssid ssid = Ssid(ssi);
        bssNode[apNodes.Get(i)->GetId()] = i;
        mac.SetType("ns3::ApWifiMac",
                    "BeaconInterval",
                    TimeValue(MicroSeconds(beaconInterval)),
                    "Ssid",
                    SsidValue(ssid));
        NetDeviceContainer tmp = wifi.Install(phy, mac, apNodes.Get(i));

        apDevices.Add(tmp.Get(0));
        devices.Add(tmp.Get(0));
        wifiNodes.Add(apNodes.Get(i));
        std::cout << "AP MAC: " << tmp.Get(0)->GetAddress() << "," << ssi << std::endl;
    }
    phy.EnablePcap("AP", apDevices);

    // std::cout << "AP addy: " << devices.Get(0)->GetAddress() << std::endl;
    // std::cout << "STA addy: " << devices.Get(i)->GetAddress() << std::endl;

    for (uint32_t i = 0; i < (apNodeCount * networkSize); ++i)
    {
        if (appType == "setup-done")
        {
            std::string configString = configuration[staNodes.Get(i)->GetId()];

            std::vector<std::string> configValues = csv_split(configString, ',');
            // for(auto it :configValues ){
            //     std::cout << "STA Values " << it << std::endl;
            // }

            std::cout << "STA node id " << staNodes.Get(i)->GetId() << " : " << configValues[0]
                      << ", " << configValues[1] << ", " << configValues[2] << ", "
                      << configValues[3] << ", " << configValues[4] << ", " << std::endl;

            double m_ccaSensitivity = std::stoi(configValues[1]);
            double m_txPower = std::stoi(configValues[2]);
            //            double m_chWidth = std::stoi(configValues[3]);
            //            double m_chNumber = std::stoi(configValues[4]);

            phy.Set("CcaSensitivity", DoubleValue(m_ccaSensitivity));
            phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel",
                                          "MinimumRssi",
                                          DoubleValue(m_ccaSensitivity));
            phy.Set("TxPowerStart", DoubleValue(m_txPower));
            phy.Set("TxPowerEnd", DoubleValue(m_txPower));

            std::string chStr = "{" + configValues[4] + "," + configValues[3] + ", BAND_5GHZ, 0}";
            phy.Set("ChannelSettings", StringValue(chStr));

            // phy.Set("ChannelWidth", UintegerValue(m_chWidth));
            //  phy.Set("ChannelNumber", UintegerValue(m_chNumber));
            //  phy.Set("Frequency", UintegerValue(frequency));
            // size_t pos = configString.find(',');
            // configString = configString.substr(pos + 1);
            // size_t pos2 = configString.find(',');
            // std::cout << "CCaSensitivity: " << configString.substr(0, pos2) << std::endl;
            // double m_ccaSensitivity = std::stoi(configString.substr(0, pos2));
            // std::cout << "apTxPower: " << configString.substr(pos2 + 1) << std::endl;
            // double m_txPower = std::stoi(configString.substr(pos2 + 1));
            // phy.Set("CcaSensitivity", DoubleValue(m_ccaSensitivity));
            // phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel",
            //                               "MinimumRssi",
            //                               DoubleValue(m_ccaSensitivity));
            // phy.Set("TxPowerStart", DoubleValue(m_txPower));
            // phy.Set("TxPowerEnd", DoubleValue(m_txPower));
        }
        // i % apNodeCount makes it so you can give each sta of the appropiate AP the correct
        // SSID
        std::string ssi = "BSS-" + std::to_string(i % apNodeCount);
        Ssid ssid = Ssid(ssi);
        bssNode[staNodes.Get(i)->GetId()] = i % apNodeCount;
        mac.SetType("ns3::StaWifiMac",
                    "MaxMissedBeacons",
                    UintegerValue(std::numeric_limits<uint32_t>::max()),
                    "Ssid",
                    SsidValue(ssid));
        NetDeviceContainer tmp = wifi.Install(phy, mac, staNodes.Get(i));

        devices.Add(tmp.Get(0));
        staDevices.Add(tmp.Get(0));
        wifiNodes.Add(staNodes.Get(i));
        // // TODO this is only according to what Hao wants to measure, this could change
        // if (i % apNodeCount == 0)
        // {
        //     // Trace PHY Rx start events
        //     Config::Connect(
        //         "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxBegin",
        //         MakeCallback(&PhyRxTrace));
        // }
        std::cout << "STA: " << i << std::endl;
        std::cout << "STA MAC: " << tmp.Get(0)->GetAddress() << "," << ssi << std::endl;
    }
    // phy.EnablePcap("STA", staDevices.Get(7));
    // phy.EnablePcap("STA", staDevices.Get(3));
    wifi.AssignStreams(devices, 0);

    // Set guard interval
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/"
                "GuardInterval",
                TimeValue(NanoSeconds(gi)));

    // Configure AP aggregation
    for (int i = 0; i < apNodeCount; ++i)
    {
        Ptr<NetDevice> dev = apNodes.Get(i)->GetDevice(0);

        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        wifi_dev->GetMac()->SetAttribute("BE_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));
        wifi_dev->GetMac()->SetAttribute("BK_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));
        wifi_dev->GetMac()->SetAttribute("VO_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));
        wifi_dev->GetMac()->SetAttribute("VI_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));

        // count associations
        wifi_dev->GetMac()->TraceConnectWithoutContext("AssociatedSta",
                                                       MakeCallback(&AssociatedSta));
        // count Desassociations
        wifi_dev->GetMac()->TraceConnectWithoutContext("DeAssociatedSta",
                                                       MakeCallback(&DeAssociatedSta));
    }
    // Configure STA aggregation
    for (uint32_t i = 0; i < (apNodeCount * networkSize); ++i)
    {
        Ptr<NetDevice> dev = staNodes.Get(i)->GetDevice(0);

        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        wifi_dev->GetMac()->SetAttribute("BE_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));
        wifi_dev->GetMac()->SetAttribute("BK_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));
        wifi_dev->GetMac()->SetAttribute("VO_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));
        wifi_dev->GetMac()->SetAttribute("VI_MaxAmpduSize",
                                         UintegerValue(maxMpdus * (pktSize + 50)));
    }

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // create a set of rooms in a building

    double xRoomCount = apNodeCount;
    double yRoomCount = 1;
    if (apNodeCount >= 3)
    {
        xRoomCount = 2;
        yRoomCount = 2;
    }

    double floorCount = 1;

    double buildingSizeX = boxSize * xRoomCount; // m
    double buildingSizeY = boxSize * yRoomCount; // m
    double buildingHeight = 3 * floorCount;      // m

    Ptr<Building> building;
    building = CreateObject<Building>();

    building->SetBoundaries(Box(0, buildingSizeX, 0, buildingSizeY, 0, buildingHeight));
    building->SetNRoomsX(xRoomCount);
    building->SetNRoomsY(yRoomCount);
    building->SetNFloors(floorCount);

    randomX->SetAttribute("Stream", IntegerValue(seedNumber));
    randomX->SetAttribute("Max", DoubleValue(boxSize));
    randomX->SetAttribute("Min", DoubleValue(0.0));

    randomY->SetAttribute("Stream", IntegerValue(seedNumber + 1));
    randomY->SetAttribute("Max", DoubleValue(boxSize));
    randomY->SetAttribute("Min", DoubleValue(0.0));
    //    double ap_x = 0;
    //    double ap_y = 0;
    // Set postion for AP
    for (uint32_t i = 0; i < apNodes.GetN(); i++)
    {
        if (ring)
        {
            double x = (boxSize / 2);
            double y = (boxSize / 2);
            if (i == 1)
            {
                x = (boxSize / 2) + (boxSize);
                y = (boxSize / 2);
            }
            if (i == 2)
            {
                x = (boxSize / 2);
                y = (boxSize / 2) + (boxSize);
            }
            else if (i == 3)
            {
                x = (boxSize / 2) + (boxSize);
                y = (boxSize / 2) + (boxSize);
            }
            Vector l1(x, y, 1.5);
            positionAlloc->Add(l1);
            std::cout << "AP" << i << " " << x << "," << y << std::endl;
            // boxOutput << "AP," << x << "," << y << std::endl;
        }
        else
        {
            double x = randomX->GetValue();
            double y = randomY->GetValue();
            if (i == 0)
            {
                //                ap_x = x;
                //                ap_y = y;
            }
            if (i == 1)
            {
                x = (boxSize / 2) + (boxSize);
                y = (boxSize / 2);
            }
            if (i == 2)
            {
                x = (boxSize / 2);
                y = (boxSize / 2) + (boxSize);
            }
            else if (i == 3)
            {
                x = (boxSize / 2) + (boxSize);
                y = (boxSize / 2) + (boxSize);
            }
            Vector l1(x, y, 1.5);
            positionAlloc->Add(l1);
            std::cout << "AP" << i << " " << x << "," << y << std::endl;
            // boxOutput << "AP," << x << "," << y << std::endl;
            // Vector l2(5.0, 5.0, 1.5);
            // std::cout << "Points intersect how many walls? " <<
            // building->WallInLOS(l1, l2)
            //           << std::endl;}
        }
    }
    std::vector<Vector> ringPos;
    if (ring)
    {
        for (uint32_t i = 0; i < networkSize; i++)
        {
            double angle = (static_cast<double>(360) / (networkSize));
            double x = (boxSize / 2) + (distance * cos(((i * angle * PI) / 180)));
            double y = (boxSize / 2) + (distance * sin(((i * angle * PI) / 180)));
            Vector l1(x, y, 1.5);
            ringPos.push_back(l1);
        }
        for (auto it : ringPos)
        {
            for (int i = 0; i < apNodeCount; i++)
            {
                double x = it.x;
                double y = it.y;
                if (i == 1)
                {
                    x = x + (boxSize);
                    //                    y = y;
                }
                if (i == 2)
                {
                    //                    x = x;
                    y = y + (boxSize);
                }
                else if (i == 3)
                {
                    x = x + (boxSize);
                    y = y + (boxSize);
                }
                Vector l1(x, y, 1.5);
                positionAlloc->Add(l1);
                std::cout << "STA" << i << " " << x << "," << y << std::endl;
                // std::cout << "STA " << (it.x) + (10 * (i % apNodeCount)) <<
                // "," << it.y
                //           << std::endl;
            }
        }
        // positionAlloc->Add(l1);
        // std::cout << "STA" << i << " " << x << "," << y << std::endl;
    }
    else
    {
        // Set postion for STAs
        for (uint32_t i = 0; i < staNodes.GetN(); i++)
        {
            double x = randomX->GetValue();
            double y = randomY->GetValue();
            double currentAp = bssNode[staNodes.Get(i)->GetId()];
            // if (i == 0)
            // {
            //     x = ((ap_x + distance) <= boxSize) ? (ap_x + distance) : (ap_x - distance);
            //     y = ((ap_y + distance) <= boxSize) ? (ap_y + distance) : (ap_y - distance);
            // }
            if (currentAp == 1)
            {
                x = x + (boxSize);
                //                y = y;
            }
            if (currentAp == 2)
            {
                //                x = x;
                y = y + (boxSize);
            }
            else if (currentAp == 3)
            {
                x = x + (boxSize);
                y = y + (boxSize);
            }
            Vector l1(x, y, 1.5);
            positionAlloc->Add(l1);
            std::cout << "STA" << i << " " << x << "," << y << std::endl;
            // std::cout << "Points intersect how many walls? " <<
            // building->WallInLOS(l1, l2)
            //           << std::endl;
        }
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiNodes);
    BuildingsHelper::Install(wifiNodes);

    if (drlCca)
    {
        Simulator::Schedule(Seconds(11), &MeasureIntervalThroughputHolDelay);
    }

    // Vector l1(7.1, 5.5, 1);
    // Vector l2(25.0, 5.5, 1);
    // // building->WallInLOS(l1, l2);
    // std::cout << "Points intersect how many walls? " <<
    // building->WallInLOS(l1, l2) << std::endl;
    if (appType == "constant")
    {
        PacketSocketHelper packetSocket;
        packetSocket.Install(wifiNodes);

        ApplicationContainer apps;
        Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable>();

        startTime->SetAttribute("Stream", IntegerValue(0));
        startTime->SetAttribute("Min", DoubleValue(6));
        startTime->SetAttribute("Max", DoubleValue(8));

        double start = 0;
        for (int i = 0; i < apNodeCount; i++)
        {
            Ptr<WifiNetDevice> wifi_apDev = DynamicCast<WifiNetDevice>(apDevices.Get(i));
            Ptr<ApWifiMac> ap_mac = DynamicCast<ApWifiMac>(wifi_apDev->GetMac());
            Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
            server->TraceConnectWithoutContext("Rx", MakeCallback(&NotifyAppRx));

            for (uint32_t x = 0; x < staNodes.GetN(); x += apNodeCount)
            {
                Ptr<WifiNetDevice> wifi_staDev = DynamicCast<WifiNetDevice>(staDevices.Get(x + i));
                Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac>(wifi_staDev->GetMac());

                std::cout << "Sta: " << staNodes.Get(x + i)->GetId() << " AP: " << i << std::endl;
                PacketSocketAddress socketAddr;
                socketAddr.SetSingleDevice(staDevices.Get((x + i))->GetIfIndex());
                socketAddr.SetPhysicalAddress(apDevices.Get(i)->GetAddress());
                socketAddr.SetProtocol(1);

                Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
                client->SetRemote(socketAddr);

                client->TraceConnectWithoutContext("Tx", MakeCallback(&NotifyAppTx));

                staNodes.Get(x + i)->AddApplication(client);
                client->SetAttribute("PacketSize", UintegerValue(pktSize));
                client->SetAttribute("MaxPackets", UintegerValue(0));
                client->SetAttribute("Interval", TimeValue(Time(MicroSeconds(pktInterval))));
                start = startTime->GetValue();
                client->SetStartTime(Seconds(start));
                std::cout << "APP START: " << start << std::endl;

                server->SetLocal(socketAddr);
            }
            apNodes.Get(i)->AddApplication(server);
        }
    }
    else if (appType == "setup-done")
    {
        Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable>();
        startTime->SetAttribute("Stream", IntegerValue(0));
        startTime->SetAttribute("Min", DoubleValue(6));
        startTime->SetAttribute("Max", DoubleValue(8));

        // PacketSocketHelper packetSocket;
        // packetSocket.Install(wifiNodes);

        ApplicationContainer apps;

        InternetStackHelper stack;
        stack.Install(wifiNodes);

        uint16_t portNumber = 50000;

        Ipv4AddressHelper address;
        address.SetBase("10.1.1.0", "255.255.255.0");

        Ipv4InterfaceContainer staInterfaces = address.Assign(staDevices);
        Ipv4InterfaceContainer apInterfaces = address.Assign(apDevices);

        for (int i = 0; i < apNodeCount; i++)
        {
            Ipv4Address apAddress = apInterfaces.GetAddress(i);
            // Create bursty application helper
            BurstyHelper burstyHelper("ns3::UdpSocketFactory",
                                      InetSocketAddress(apAddress, portNumber));
            burstyHelper.SetAttribute("FragmentSize", UintegerValue(pktSize));

            // Create burst sink helper
            BurstSinkHelper burstSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(apAddress, portNumber));

            // Install burst sink
            ApplicationContainer apApps = burstSinkHelper.Install(apNodes.Get(i));
            Ptr<BurstSink> burstSink = apApps.Get(0)->GetObject<BurstSink>();

            for (uint32_t x = 0; x < staNodes.GetN(); x += apNodeCount)
            {
                std::string trafficType = configuration[staNodes.Get(x + i)->GetId()];
                size_t pos = trafficType.find(',');
                trafficType = trafficType.substr(0, pos);
                std::cout << "Sta: " << staNodes.Get(x + i)->GetId() << " Traffic " << trafficType
                          << std::endl;
                if (trafficType == "constant")
                {
                    Ptr<WifiNetDevice> wifi_apDev = DynamicCast<WifiNetDevice>(apDevices.Get(i));
                    Ptr<ApWifiMac> ap_mac = DynamicCast<ApWifiMac>(wifi_apDev->GetMac());
                    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
                    server->TraceConnectWithoutContext("Rx", MakeCallback(&NotifyAppRx));
                    Ptr<WifiNetDevice> wifi_staDev =
                        DynamicCast<WifiNetDevice>(staDevices.Get(x + i));
                    Ptr<StaWifiMac> sta_mac = DynamicCast<StaWifiMac>(wifi_staDev->GetMac());

                    // std::cout << "Sta: " << staNodes.Get(x + i)->GetId() << " AP: " << i
                    //           << std::endl;
                    PacketSocketAddress socketAddr;
                    socketAddr.SetSingleDevice(staDevices.Get((x + i))->GetIfIndex());
                    socketAddr.SetPhysicalAddress(apDevices.Get(i)->GetAddress());
                    socketAddr.SetProtocol(1);

                    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
                    client->SetRemote(socketAddr);

                    client->TraceConnectWithoutContext("Tx", MakeCallback(&NotifyAppTx));

                    staNodes.Get(x + i)->AddApplication(client);
                    client->SetAttribute("PacketSize", UintegerValue(pktSize));
                    client->SetAttribute("MaxPackets", UintegerValue(0));
                    client->SetAttribute("Interval", TimeValue(Time(MicroSeconds(pktInterval))));

                    client->SetStartTime(Seconds(startTime->GetValue()));
                    // std::cout << "APP START: " << start << std::endl;

                    server->SetLocal(socketAddr);
                    if (x == 0)
                    {
                        apNodes.Get(i)->AddApplication(server);
                    }
                }
                else if (trafficType == "bursty")
                {
                    // Example of connecting to the trace sources
                    burstSink->TraceConnectWithoutContext("BurstRx", MakeCallback(&BurstRx));
                    burstSink->TraceConnectWithoutContext("FragmentRx", MakeCallback(&FragmentRx));
                    burstyHelper.SetBurstGenerator(
                        "ns3::SimpleBurstGenerator",
                        "PeriodRv",
                        StringValue("ns3::ConstantRandomVariable[Constant=5e-3]"),
                        "BurstSizeRv",
                        StringValue("ns3::ConstantRandomVariable[Constant=25e3]"));

                    // Install bursty application
                    ApplicationContainer staApps = burstyHelper.Install(staNodes.Get(i + x));
                    Ptr<BurstyApplication> burstyApp =
                        staApps.Get(0)->GetObject<BurstyApplication>();

                    // Example of connecting to the trace sources
                    burstyApp->TraceConnectWithoutContext("FragmentTx", MakeCallback(&FragmentTx));
                    burstyApp->TraceConnectWithoutContext("BurstTx", MakeCallback(&BurstTx));
                }
                else if (trafficType == "none")
                {
                    continue;
                }
                else
                {
                    size_t pos = trafficType.find('-');
                    traceFile = trafficType.substr(pos + 1);
                    // std::cout << traceFile << std::endl;
                    burstSink->TraceConnectWithoutContext("BurstRx", MakeCallback(&BurstRx));
                    burstSink->TraceConnectWithoutContext("FragmentRx", MakeCallback(&FragmentRx));
                    burstyHelper.SetBurstGenerator("ns3::TraceFileBurstGenerator",
                                                   "TraceFile",
                                                   StringValue(traceFolder + traceFile),
                                                   "StartTime",
                                                   DoubleValue(startTime->GetValue()));

                    // Install bursty application
                    ApplicationContainer staApps = burstyHelper.Install(staNodes.Get(i + x));
                    Ptr<BurstyApplication> burstyApp =
                        staApps.Get(0)->GetObject<BurstyApplication>();

                    // Extract TraceFileBurstGenerator and check if able to fill the entire
                    // simulation
                    PointerValue val;
                    burstyApp->GetAttribute("BurstGenerator", val);
                    Ptr<TraceFileBurstGenerator> tfbg =
                        DynamicCast<TraceFileBurstGenerator>(val.GetObject());
                    NS_ASSERT_MSG(tfbg,
                                  "The bursty application should be a TraceFileBurstGenerator");

                    if (((10) + duration) > tfbg->GetTraceDuration())
                    {
                        NS_ABORT_MSG(
                            "The trace file will end before the simulation ends. Please choose "
                            "a "
                            "different start time, a longer trace, or reduce the simulation "
                            "duration.");
                    }

                    // Example of connecting to the trace sources
                    burstyApp->TraceConnectWithoutContext("FragmentTx", MakeCallback(&FragmentTx));
                    burstyApp->TraceConnectWithoutContext("BurstTx", MakeCallback(&BurstTx));
                }
            }
        }
    }
    else
    {
        InternetStackHelper stack;
        stack.Install(wifiNodes);

        uint16_t portNumber = 50000;

        Ipv4AddressHelper address;
        address.SetBase("10.1.1.0", "255.255.255.0");

        Ipv4InterfaceContainer staInterfaces = address.Assign(staDevices);
        Ipv4InterfaceContainer apInterfaces = address.Assign(apDevices);

        Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable>();

        startTime->SetAttribute("Stream", IntegerValue(0));
        startTime->SetAttribute("Min", DoubleValue(6));
        startTime->SetAttribute("Max", DoubleValue(8));

        for (int i = 0; i < apNodeCount; i++)
        {
            Ipv4Address apAddress = apInterfaces.GetAddress(i);
            // Create bursty application helper
            BurstyHelper burstyHelper("ns3::UdpSocketFactory",
                                      InetSocketAddress(apAddress, portNumber));
            burstyHelper.SetAttribute("FragmentSize", UintegerValue(pktSize));

            // Create burst sink helper
            BurstSinkHelper burstSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(apAddress, portNumber));

            // Install burst sink
            ApplicationContainer apApps = burstSinkHelper.Install(apNodes.Get(i));
            Ptr<BurstSink> burstSink = apApps.Get(0)->GetObject<BurstSink>();

            // Example of connecting to the trace sources
            burstSink->TraceConnectWithoutContext("BurstRx", MakeCallback(&BurstRx));
            burstSink->TraceConnectWithoutContext("FragmentRx", MakeCallback(&FragmentRx));
            for (uint32_t x = 0; x < staNodes.GetN(); x += apNodeCount)
            {
                // Ipv4Address staAddress = staInterfaces.GetAddress(x + i); // 0.0.0.0
                std::cout << "Sta: " << staNodes.Get(x + i)->GetId() << " AP: " << i << std::endl;
                if (appType == "bursty")
                {
                    burstyHelper.SetBurstGenerator(
                        "ns3::SimpleBurstGenerator",
                        "PeriodRv",
                        StringValue("ns3::ConstantRandomVariable[Constant=100e-3]"),
                        "BurstSizeRv",
                        StringValue("ns3::ConstantRandomVariable[Constant=10e3]"));
                }
                else if (appType == "bursty-trace")
                {
                    burstyHelper.SetBurstGenerator("ns3::TraceFileBurstGenerator",
                                                   "TraceFile",
                                                   StringValue(traceFolder + traceFile),
                                                   "StartTime",
                                                   DoubleValue(startTime->GetValue()));
                }
                // Install bursty application
                ApplicationContainer staApps = burstyHelper.Install(staNodes.Get(i + x));
                Ptr<BurstyApplication> burstyApp = staApps.Get(0)->GetObject<BurstyApplication>();
                if (appType == "bursty-trace")
                {
                    // Extract TraceFileBurstGenerator and check if able to fill the entire
                    // simulation
                    PointerValue val;
                    burstyApp->GetAttribute("BurstGenerator", val);
                    Ptr<TraceFileBurstGenerator> tfbg =
                        DynamicCast<TraceFileBurstGenerator>(val.GetObject());
                    NS_ASSERT_MSG(tfbg,
                                  "The bursty application should be a TraceFileBurstGenerator");

                    if (((10) + duration) > tfbg->GetTraceDuration())
                    {
                        NS_ABORT_MSG(
                            "The trace file will end before the simulation ends. Please choose "
                            "a "
                            "different start time, a longer trace, or reduce the simulation "
                            "duration.");
                    }
                }
                // Example of connecting to the trace sources
                burstyApp->TraceConnectWithoutContext("FragmentTx", MakeCallback(&FragmentTx));
                burstyApp->TraceConnectWithoutContext("BurstTx", MakeCallback(&BurstTx));

                // Stop bursty app after simTimeSec
                // staApps.Stop(Seconds(simTimeSec));
            }
        }
    }

    Simulator::Schedule(Seconds(0), &StartStatistics);
    // populate m_staMacAddressToNodeId map
    for (auto it = staDevices.Begin(); it != staDevices.End(); it++)
    {
        m_staMacAddressToNodeId[Mac48Address::ConvertFrom((*it)->GetAddress())] =
            (*it)->GetNode()->GetId();
    }
    for (int i = 0; i < apNodeCount; ++i)
    {
        // Log packet receptions
        std::string configPath = "/NodeList/" + std::to_string(apNodes.Get(i)->GetId()) +
                                 "/DeviceList/*/$ns3::WifiNetDevice/Phys/*/$ns3::WifiPhy/"
                                 "MonitorSnifferRx";
        Config::Connect(configPath, MakeCallback(&TracePacketReception));

        // Log packet drops
        configPath = "/NodeList/" + std::to_string(apNodes.Get(i)->GetId()) +
                     "/DeviceList/*/$ns3::WifiNetDevice/Phys/*/$ns3::WifiPhy/"
                     "PhyRxDrop";
        Config::Connect(configPath, MakeCallback(&PhyDrop));

        // Log packet reception
        configPath = "/NodeList/" + std::to_string(apNodes.Get(i)->GetId()) +
                     "/DeviceList/*/$ns3::WifiNetDevice/Phys/*/$ns3::WifiPhy/"
                     "PhyRxEnd";
        Config::Connect(configPath, MakeCallback(&PhyEnd));
    }
    if (autoMCS)
    {
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/"
                        "$ns3::AutoMcsWifiManager/Rate",
                        MakeCallback(&RateChange));
    }
    else
    {
        for (size_t i = 0; i < wifiNodes.GetN(); i++)
        {
            nodeMcs[wifiNodes.Get(i)->GetId()] = mcs;
        }
    }

    // Trace CW evolution

    Config::Connect(
        "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::WifiMac/BE_Txop/CwTrace",
        MakeCallback(&CwTrace));
    // Trace backoff evolution

    Config::Connect(
        "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::WifiMac/BE_Txop/BackoffTrace",
        MakeCallback(&BackoffTrace));

    // Trace PHY Tx begin events
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxBegin",
                    MakeCallback(&NotifyPhyTxBegin));

    // Trace PHY Tx end events
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxEnd",
                    MakeCallback(&PhyTxDoneTrace));

    // Trace PHY Tx drop events
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxDrop",
                    MakeCallback(&PhyTxDropTrace));

    // // Trace CW evolution

    // Config::Connect(
    //     "/NodeList/9/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::WifiMac/BE_Txop/CwTrace",
    //     MakeCallback(&CwTrace));
    // // Trace backoff evolution

    // Config::Connect(
    //     "/NodeList/9/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::WifiMac/BE_Txop/BackoffTrace",
    //     MakeCallback(&BackoffTrace));

    // Simulator::Schedule(Seconds(10), &ChangeCcaSensitivity, 1, 1);
    Simulator::Schedule(Seconds(10), &RestartIntervalThroughputHolDelay);
    Simulator::Schedule(Seconds(1.5), &CheckAssociation);
    Simulator::Schedule(Seconds(10), &RestartCalc);
    //    Simulator::Schedule(Seconds(10), &TrackTime);
    Simulator::Stop(Seconds((10) + duration));
    Simulator::Run();

    std::ostream& os = std::cout;

    // for (auto it : nodeBackoff)
    // {
    //     double backoff = 0;
    //     int count = 0;
    //     for (auto value : it.second)
    //     {
    //         backoff += value;
    //         count++;
    //     }
    //     os << "Node" << it.first << "AVG Backoff: " << backoff / count << std::endl;
    // }

    // for (auto it : nodeCw)
    // {
    //     double cwT = 0;
    //     int count = 0;
    //     for (auto value : it.second)
    //     {
    //         cwT += value;
    //         count++;
    //     }
    //     os << "Node" << it.first << "AVG ContentionWindow: " << cwT / count << std::endl;
    // }
    // double cwT = 0;
    // for (auto it : nodeCw)
    // {
    //     cwT += it;
    // }
    // os << "Node9 CW: " << cwT << std::endl;
    // os << "Node9 Backoff: " << backoff << std::endl;

    // backoff = 0;
    // for (auto it : nodeBackoff[5])
    // {
    //     backoff += it;
    // }
    // cwT = 0;
    // for (auto it : nodeBackoff[5])
    // {
    //     cwT += it;
    // }
    // os << "Node5 CW: " << cwT << std::endl;
    // os << "Node5 Backoff: " << backoff << std::endl;

    // for (auto& nodeEntry : nodePacketTxTime)
    // {
    //     uint32_t nodeID = nodeEntry.first;
    //     std::map<uint64_t, std::vector<Time>>& eventMap = nodeEntry.second;

    //     for (auto& eventEntry : eventMap)
    //     {
    //         uint64_t eventID = eventEntry.first;
    //         std::vector<Time>& eventTimes = eventEntry.second;
    //         std::vector<Time>& eventEndTimes = nodePacketTxEndTime[nodeID][eventID];
    //         if ((eventTimes.size() != eventEndTimes.size()))
    //         {
    //             std::cout << "diff amount of times " << eventTimes.size() << " and "
    //                       << eventEndTimes.size() << std::endl;
    //             nodePacketTxEndTime[nodeID]->erase(eventID);
    //             continue;
    //         }
    //         // for (size_t i = 0; i < eventTimes.size(); i++)
    //         // {
    //         //     std::cout << "Packet: " << eventID << " Start: " <<
    //         eventTimes[i].GetSeconds()
    //         //               << std::endl;
    //         //     std::cout << "Packet: " << eventID << " end: " <<
    //         eventEndTimes[i].GetSeconds()
    //         //               << std::endl;
    //         // }
    //     }
    // }

    std::cout << "\n" << std::endl;
    if (calculateStats)
    {
        createPacketPairs();
        overlappingPackets other;
        int interBssCollissionsFails = 0;
        int intraBssCollissionsFails = 0;
        for (auto packet : packetOverlapList)
        {
            // Skip two entries that share everything except the interferer packet and
            // interferer node. It is possible that the ifPacket are different but it is the same
            // entry
            if ((packet.nodeID == other.nodeID) && (packet.rxNodeID == other.rxNodeID) &&
                (packet.packet == other.packet) && (packet.reason == other.reason) &&
                (packet.phyDropTime == other.phyDropTime))
            {
                // std::cout << "SKIP" << std::endl;
                // typeOverlapCount[packet.reason] += 1;
                continue;
            }
            other = packet;
            int bss1 = bssNode[packet.nodeID];
            int bss2 = bssNode[packet.ifNodeID];
            // std::cout << "Tx Node BSS" << bss1 << " IF Node BSS" << bss2 << std::endl;
            if (bss1 != bss2)
            {
                interBssCollissionsFails++;
            }
            else
            {
                intraBssCollissionsFails++;
            }
            typeOverlapCount[packet.reason] += 1;
            totalDropsByOverlap++;
            // std::cout << "Node " << packet.nodeID << " Tx to Node " << packet.rxNodeID
            //           << " the packet " << packet.packet << " and it overlapped " << packet.sync
            //           << " with packet " << packet.ifPacket << " from Node " << packet.ifNodeID
            //           << "\nCausing the first packet to drop due to " << packet.reason
            //           << " at T= " << packet.phyDropTime.GetSeconds() << "\n"
            //           << std::endl;
        }
        int sucessfullSimulTx = 0;
        int interBssCollissionsSuccess = 0;
        int intraBssCollissionsSuccess = 0;
        overlappingPackets other2;
        for (auto packet : packetOverlapSuccessList)
        {
            if ((packet.packet == other2.packet) && (packet.nodeID == other2.nodeID) &&
                (packet.startTime == other2.startTime))
            {
                // std::cout << "SKIP" << std::endl;
                // typeOverlapCount[packet.reason] += 1;
                continue;
            }
            other2 = packet;
            int bss1 = bssNode[packet.nodeID];
            int bss2 = bssNode[packet.ifNodeID];
            // std::cout << "Tx Node BSS" << bss1 << " IF Node BSS" << bss2 << std::endl;
            if (bss1 != bss2)
            {
                interBssCollissionsSuccess++;
            }
            else
            {
                intraBssCollissionsSuccess++;

                std::cout << "Node " << other2.nodeID << " Tx the packet " << other2.packet
                          << " and it overlapped " << other2.sync << " with packet "
                          << other2.ifPacket << " from Node " << other2.ifNodeID
                          << "\n but the first packet did not get dropped"
                          << ".It was succesfully received at T= "
                          << other2.phyDropTime.GetSeconds() << "\n"
                          << std::endl;
            }
            // std::cout << "Node " << other.nodeID << " Tx the packet " << other.packet
            //           << " and it overlapped " << other.sync << " with packet " << other.ifPacket
            //           << " from Node " << other.ifNodeID
            //           << "\n but the first packet did not get dropped "
            //           << ".It was succesfully received at T= " << other.phyDropTime.GetSeconds()
            //           << "\n"
            //           << std::endl;
            sucessfullSimulTx++;
        }

        // int totalTx = 0;
        // for (auto nodePacketTime : nodePacketTxTime)
        // {
        //     for (auto nodePacket : nodePacketTime.second)
        //     {
        //         totalTx += nodePacket.second.size();
        //     }
        // }

        // for (auto packet : packetOverlapSuccessList)
        // {
        //     sucessfullSimulTx++;
        // }

        for (auto fails : typeFailCount)
        {
            std::cout << "Failure Reason " << fails.first << " count " << fails.second << std::endl;
        }
        //        int failedSimulTx = 0;
        //        for (auto fails : typeOverlapCount)
        //        {
        //            failedSimulTx += fails.second;
        //        }

        for (auto fails : typeOverlapCount)
        {
            std::cout << "\nOverlap Reason " << fails.first << " "
                      << (fails.second * 100.0) / (totalDropsByOverlap + sucessfullSimulTx) << "%"
                      << " Count " << fails.second << std::endl;
            //            failedSimulTx += fails.second;
        }
        std::cout << "Total Tx: " << totalTx << std::endl;
        std::cout << "Total Simultaneus Tx: " << totalDropsByOverlap + sucessfullSimulTx << " "
                  << ((totalDropsByOverlap + sucessfullSimulTx) * 100) / totalTx << "%"
                  << std::endl;
        std::cout << "Total Succesful Simultaneus Tx: " << sucessfullSimulTx << std::endl;
        std::cout << "Total Failed Simultaneus Tx: " << totalDropsByOverlap << " "
                  << (totalDropsByOverlap * 100.0) / (totalDropsByOverlap + sucessfullSimulTx)
                  << "%" << std::endl;

        std::cout << "Succesfull \nIntraBss Collisions: " << intraBssCollissionsSuccess << " "
                  << (intraBssCollissionsSuccess * 100.0) /
                         (totalDropsByOverlap + sucessfullSimulTx)
                  << "%"
                  << "\nInterBss Collisions: " << interBssCollissionsSuccess << " "
                  << (interBssCollissionsSuccess * 100.0) /
                         (totalDropsByOverlap + sucessfullSimulTx)
                  << "%" << std::endl;

        // std::cout << "Succesfull \nIntraBss Collisions: "
        //           << (intraBssCollissionsSuccess * 100) / (totalDropsByOverlap +
        //           sucessfullSimulTx)
        //           << "\nInterBss Collisions: "
        //           << (interBssCollissionsSuccess * 100) / (totalDropsByOverlap +
        //           sucessfullSimulTx)
        //           << std::endl;
        // std::cout << "Succesfull \nIntraBss Collisions: "
        //           << (intraBssCollissionsSuccess * 100) / (totalDropsByOverlap +
        //           sucessfullSimulTx)
        //           << "\nInterBss Collisions: "
        //           << (interBssCollissionsSuccess * 100) / (totalDropsByOverlap +
        //           sucessfullSimulTx)
        //           << std::endl;
        std::cout << "Failures \nIntraBss Collisions: " << intraBssCollissionsFails
                  << "\nInterBss Collisions: " << interBssCollissionsFails << std::endl;
    }
    std::cout << "\n" << std::endl;
    double throughput = 0;
    double rPackets = 0;

    // for (auto nodePacketTime : nodePacketTxTime)
    // {
    //     for (auto packetTime : nodePacketTime.second)
    //     {
    //         double startT = 0;
    //         double endT = 0;

    //         for (auto t : packetTime.second)
    //         {
    //             if (startT == 0)
    //             {
    //                 startT = t.GetSeconds();
    //             }
    //             else
    //             {
    //                 endT = t.GetSeconds();
    //             }
    //             // std::cout << "packet " << packetTime.first << " times: " << t <<
    //             std::endl; if (endT != 0)
    //             {
    //                 std::cout << "Node " << nodePacketTime.first << " packet " <<
    //                 packetTime.first
    //                           << " start: " << startT << " end: " << endT << std::endl;
    //             }
    //         }

    //         if (endT == 0)
    //         {
    //             nodePacketTime.second.erase(packetTime.first);
    //         }
    //     }
    // }

    for (auto it = timeFirstReceived.begin(); it != timeFirstReceived.end(); it++)
    {
        Time first = it->second;
        Time last = timeLastReceived.find(it->first)->second;
        Time dataTransferDuration = last - first;
        if (dataTransferDuration.GetSeconds() <= 0)
        {
            os << "Link " << MacAddressToNodeId(it->first) << " Throughput: 0" << std::endl;
            continue;
        }
        double node_thru = (bytesReceived.find(it->first)->second * 8) /
                           static_cast<double>(dataTransferDuration.GetMicroSeconds());
        rPackets += packetsReceived.find(it->first)->second;

        throughput += node_thru;

        os << "Node " << MacAddressToNodeId(it->first) << " Aggregated Throughput: " << node_thru
           << std::endl;
    }
    // std::cout << "percentage lost: " << ((rPackets + drops) / (drops * 100))
    // << std::endl;
    os << "MCS value"
       << "\t\t"
       << "Channel width"
       << "\t\t"
       << "GI"
       << "\t\t\t"
       << "Throughput" << '\n';
    os << mcs << "\t\t\t" << channelWidth << " MHz\t\t\t" << gi << " ns\t\t\t" << throughput
       << " Mbit/s" << std::endl;
    // Create a csv file to store the results
    std::ofstream out;
    out.open("results.csv", std::ios::trunc);
    out << "srcNodeId,"
        << "pktSize,"
        << "lastPacketTime,"
        << "dequeueTime,"
        << "HOL,"
        << "queuingDelay,"
        << "accessDelay,"
        << "txDelay"
        << "\n";

    for (auto mapIt : m_inFlightPacketMap)
    {
        for (auto listIt : mapIt.second)
        {
            if (listIt.m_dequeued)
            {
                uint32_t srcNodeId = MacAddressToNodeId(listIt.m_srcAddress);
                out << srcNodeId << "," << listIt.m_ptrToPacket->GetSize() << ","
                    << listIt.m_HoLTime << "," << listIt.m_edcaDequeueTime << ","
                    << (listIt.m_edcaDequeueTime - listIt.m_HoLTime).ToDouble(Time::MS) << ","
                    << (listIt.m_HoLTime - listIt.m_edcaEnqueueTime).ToDouble(Time::MS) << ","
                    << (listIt.m_phyTxTime - listIt.m_HoLTime).ToDouble(Time::MS) << ","
                    << (listIt.m_edcaDequeueTime - listIt.m_phyTxTime).ToDouble(Time::MS) << ","
                    << std::endl;
            }
        }
    }

    for (auto it : edcaHolSample)
    {
        uint32_t srcNodeId = it.first;
        double sum1 = 0;
        double sum2 = 0;
        double sum3 = 0;
        double sum4 = 0;

        for (double i = 0; i < it.second.size(); i += 4)
        {
            sum1 += it.second[i];
            sum2 += it.second[i + 1];
            sum3 += it.second[i + 2];
            sum4 += it.second[i + 3];
        }

        os << "NodeID: " << srcNodeId << " \n Average HoLd: " << sum1 / (it.second.size() / 4)
           << "ms"
           << " \n Average Queuing Delay: " << sum2 / (it.second.size() / 4) << "ms"
           << " \n Average Access Delay: " << sum3 / (it.second.size() / 4) << "ms"
           << " \n Average Tx Delay: " << sum4 / (it.second.size() / 4) << "ms" << std::endl;
        // os << "Size: " << it.second.size() << "\n";
    }
    os << "\n";
    out.close();
    std::cout << "PHYDROPs: " << drops << std::endl;
    std::cout << "PHYDROP counted as overlap: " << totalDropsByOverlap << std::endl;
    std::cout << "ReceivedPackets: " << rPackets << std::endl;
    std::cout << "AppReceivedPackets: " << appTxrec << std::endl;
    // std::cout << "PHY Receives: " << receives << std::endl;

    out.open("test.csv", std::ios::app);
    // out << "phyMode,ccaSensitivity,throughput" << std::endl;
    out << phyMode << "," << ccaSensitivity << "," << throughput << std::endl;
    out.close();
    std::cout << "Aggregated Throughput: " << throughput << std::endl;
    if (deassociatedStas > 0)
    {
        NS_ABORT_MSG("There was a station dessasociated");
    }
    //    if (drlCca)
    //    {
    //        rlAlgo.SetFinish();
    //    }
    PrintPythonPlotCSV("box.csv");
    Simulator::Destroy();
    return 0;
}
