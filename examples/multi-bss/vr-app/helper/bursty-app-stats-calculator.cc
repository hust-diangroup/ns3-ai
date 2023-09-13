/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "bursty-app-stats-calculator.h"

#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/nstime.h"
#include "ns3/seq-ts-size-frag-header.h"
#include "ns3/string.h"
#include <ns3/boolean.h>
#include <ns3/log.h>

#include <algorithm>
#include <iomanip>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BurstyAppStatsCalculator");

NS_OBJECT_ENSURE_REGISTERED(BurstyAppStatsCalculator);

BurstyAppStatsCalculator::BurstyAppStatsCalculator()
    : m_firstWrite(true),
      m_pendingOutput(false),
      m_aggregatedStats(true)
{
    NS_LOG_FUNCTION(this);
}

BurstyAppStatsCalculator::~BurstyAppStatsCalculator()
{
    NS_LOG_FUNCTION(this);
}

TypeId
BurstyAppStatsCalculator::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::BurstyAppStatsCalculator")
            .SetParent<Object>()
            .SetGroupName("Applications")
            .AddConstructor<BurstyAppStatsCalculator>()
            .AddAttribute("StartTime",
                          "Start time of the on going epoch.",
                          TimeValue(Seconds(0.)),
                          MakeTimeAccessor(&BurstyAppStatsCalculator::SetStartTime,
                                           &BurstyAppStatsCalculator::GetStartTime),
                          MakeTimeChecker())
            .AddAttribute("EpochDuration",
                          "Epoch duration.",
                          TimeValue(Seconds(0.25)),
                          MakeTimeAccessor(&BurstyAppStatsCalculator::GetEpoch,
                                           &BurstyAppStatsCalculator::SetEpoch),
                          MakeTimeChecker())
            .AddAttribute("OutputFilename",
                          "Name of the file where the downlink results will be saved.",
                          StringValue("AppStats.txt"),
                          MakeStringAccessor(&BurstyAppStatsCalculator::SetOutputFilename,
                                             &BurstyAppStatsCalculator::GetOutputFilename),
                          MakeStringChecker())
            .AddAttribute("AggregatedStats",
                          "Choice to show the results aggregated of disaggregated. As of now, "
                          "non-aggregated stats are not supported",
                          BooleanValue(true),
                          MakeBooleanAccessor(&BurstyAppStatsCalculator::m_aggregatedStats),
                          MakeBooleanChecker())
            .AddAttribute("ManualUpdate",
                          "Choice to perform manual statistics update, e.g., triggered by an "
                          "external class.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BurstyAppStatsCalculator::GetManualUpdate,
                                              &BurstyAppStatsCalculator::SetManualUpdate),
                          MakeBooleanChecker())
            .AddAttribute("WriteToFile",
                          "Choice to write stats to file besides computing and exchange them "
                          "with external classes",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BurstyAppStatsCalculator::m_writeToFile),
                          MakeBooleanChecker());
    return tid;
}

void
BurstyAppStatsCalculator::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
BurstyAppStatsCalculator::SetStartTime(Time t)
{
    m_startTime = t;
    if (m_aggregatedStats)
    {
        RescheduleEndEpoch();
    }
}

Time
BurstyAppStatsCalculator::GetStartTime() const
{
    return m_startTime;
}

void
BurstyAppStatsCalculator::SetEpoch(Time e)
{
    m_epochDuration = e;
    if (m_aggregatedStats)
    {
        RescheduleEndEpoch();
    }
}

Time
BurstyAppStatsCalculator::GetEpoch() const
{
    return m_epochDuration;
}

void
BurstyAppStatsCalculator::SetManualUpdate(bool u)
{
    NS_LOG_FUNCTION(this);
    m_manualUpdate = u;
    if (m_manualUpdate)
    {
        NS_LOG_UNCOND("Cancel EndEpoch event");
        m_endEpochEvent.Cancel();
    }
}

bool
BurstyAppStatsCalculator::GetManualUpdate() const
{
    NS_LOG_FUNCTION(this);
    return m_manualUpdate;
}

void
BurstyAppStatsCalculator::TxBurst(uint32_t nodeId,
                                  Ptr<const Packet> burst,
                                  const Address& from,
                                  const Address& to,
                                  const SeqTsSizeFragHeader& header)
{
    NS_LOG_FUNCTION(this << " TxBurst nodeId=" << nodeId << " burst seq=" << header.GetSeq()
                         << " of " << header.GetSize() << " bytes transmitted at "
                         << std::setprecision(9) << header.GetTs().As(Time::S));

    if (m_aggregatedStats)
    {
        if (Simulator::Now() >= m_startTime)
        {
            m_txBursts[nodeId]++;
            m_txData[nodeId] += header.GetSize();
        }
        m_pendingOutput = true;
    }
}

void
BurstyAppStatsCalculator::RxBurst(uint32_t nodeId,
                                  Ptr<const Packet> burst,
                                  const Address& from,
                                  const Address& to,
                                  const SeqTsSizeFragHeader& header)
{
    NS_LOG_FUNCTION(this << " RxBurst nodeId=" << nodeId << " burst seq=" << header.GetSeq()
                         << " of " << header.GetSize() << " bytes transmitted at "
                         << std::setprecision(9) << header.GetTs().As(Time::S));
    if (m_aggregatedStats)
    {
        if (Simulator::Now() >= m_startTime)
        {
            m_rxBursts[nodeId]++;
            m_rxData[nodeId] += header.GetSize();

            auto it = m_delay.find(nodeId);
            if (it == m_delay.end())
            {
                NS_LOG_DEBUG(this << " Creating delay stats calculator for node " << nodeId);
                m_delay[nodeId] = CreateObject<MinMaxAvgTotalCalculator<uint64_t>>();
            }

            uint64_t delay = Simulator::Now().GetNanoSeconds() - header.GetTs().GetNanoSeconds();
            m_delay[nodeId]->Update(delay);
        }
        m_pendingOutput = true;
    }
}

std::map<uint16_t, AppResults>
BurstyAppStatsCalculator::ReadResults(void)
{
    NS_LOG_FUNCTION(this);

    // Get the list of node IDs
    std::vector<uint32_t> nodeIdsVector;
    for (auto it = m_txBursts.begin(); it != m_txBursts.end(); ++it)
    {
        if (find(nodeIdsVector.begin(), nodeIdsVector.end(), (*it).first) == nodeIdsVector.end())
        {
            nodeIdsVector.push_back((*it).first);
        }
    }

    std::map<uint16_t, AppResults> results;
    for (auto it = nodeIdsVector.begin(); it != nodeIdsVector.end(); ++it)
    {
        AppResults item;
        uint32_t nodeId = *it;
        item.imsi = nodeId;

        item.txBursts = m_txBursts[nodeId];
        item.txData = m_txData[nodeId];

        item.rxBursts = m_rxBursts[nodeId];
        item.rxData = m_rxData[nodeId];

        auto iter = m_delay.find(nodeId);

        // if no delay info have been recorded yet, put it to zero
        if (iter == m_delay.end())
        {
            item.delayMean = 0.0;
            item.delayStdev = 0.0;
            item.delayMin = 0.0;
            item.delayMax = 0.0;
        }
        else
        {
            item.delayMean = m_delay[nodeId]->getMean();
            item.delayStdev = m_delay[nodeId]->getStddev();
            item.delayMin = m_delay[nodeId]->getMin();
            item.delayMax = m_delay[nodeId]->getMax();
        }
        results.insert(std::make_pair(item.imsi, item));
    }
    if (m_writeToFile)
    {
        ShowResults();
    }
    ResetResults();
    return results;
}

void
BurstyAppStatsCalculator::ShowResults(void)
{
    std::ofstream outFile;

    if (m_firstWrite == true)
    {
        outFile.open(GetOutputFilename().c_str());
        if (!outFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetOutputFilename().c_str());
            return;
        }

        m_firstWrite = false;
        outFile << "start\tend\tNodeId\tnTxBursts\tTxBytes\tnRxBursts\tRxBytes\tdelay\tstdDev\tmin"
                   "\tmax\t";
        outFile << std::endl;
    }
    else
    {
        outFile.open(GetOutputFilename().c_str(), std::ios_base::app);
        if (!outFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetOutputFilename().c_str());
            return;
        }
    }

    WriteResults(outFile);
    m_pendingOutput = false;
}

void
BurstyAppStatsCalculator::WriteResults(std::ofstream& outFile)
{
    NS_LOG_FUNCTION(this);

    // Get the list of node IDs

    std::vector<uint32_t> nodeIdsVector;
    for (auto it = m_txBursts.begin(); it != m_txBursts.end(); ++it)
    {
        if (find(nodeIdsVector.begin(), nodeIdsVector.end(), (*it).first) == nodeIdsVector.end())
        {
            nodeIdsVector.push_back((*it).first);
        }
    }

    // Check if there is some ID missing
    for (auto it = m_rxBursts.begin(); it != m_rxBursts.end(); ++it)
    {
        if (find(nodeIdsVector.begin(), nodeIdsVector.end(), (*it).first) == nodeIdsVector.end())
        {
            nodeIdsVector.push_back((*it).first);
        }
    }

    Time endTime;
    if (m_manualUpdate)
    {
        endTime = Simulator::Now();
    }
    else
    {
        endTime = m_startTime + m_epochDuration;
    }

    for (auto it = nodeIdsVector.begin(); it != nodeIdsVector.end(); ++it)
    {
        uint32_t nodeId = *it;
        outFile << m_startTime.GetNanoSeconds() / 1.0e9 << "\t";
        outFile << endTime.GetNanoSeconds() / 1.0e9 << "\t";

        outFile << nodeId << "\t";

        outFile << m_txBursts[nodeId] << "\t";
        outFile << m_txData[nodeId] << "\t";

        outFile << m_rxBursts[nodeId] << "\t";
        outFile << m_rxData[nodeId] << "\t";

        auto iter = m_delay.find(nodeId);

        // if no delay info have been recorded yet, put it to zero
        if (iter == m_delay.end())
        {
            outFile << 0.0 << "\t";
            outFile << 0.0 << "\t";
            outFile << 0.0 << "\t";
            outFile << 0.0 << "\t";
        }
        else
        {
            outFile << m_delay[nodeId]->getMean() << "\t";
            outFile << m_delay[nodeId]->getStddev() << "\t";
            outFile << m_delay[nodeId]->getMin() << "\t";
            outFile << m_delay[nodeId]->getMax() << "\t";
        }

        outFile << std::endl;
    }

    outFile.close();
}

void
BurstyAppStatsCalculator::ResetResults(void)
{
    NS_LOG_FUNCTION(this);

    m_rxBursts.erase(m_rxBursts.begin(), m_rxBursts.end());
    m_txBursts.erase(m_txBursts.begin(), m_txBursts.end());

    m_txData.erase(m_txData.begin(), m_txData.end());
    m_rxData.erase(m_rxData.begin(), m_rxData.end());

    m_delay.erase(m_delay.begin(), m_delay.end());
}

void
BurstyAppStatsCalculator::RescheduleEndEpoch(void)
{
    NS_LOG_FUNCTION(this);
    m_endEpochEvent.Cancel();
    NS_ASSERT(Simulator::Now().GetMilliSeconds() == 0); // below event time assumes this
    m_endEpochEvent = Simulator::Schedule(m_startTime + m_epochDuration,
                                          &BurstyAppStatsCalculator::EndEpoch,
                                          this);
}

void
BurstyAppStatsCalculator::EndEpoch(void)
{
    NS_LOG_FUNCTION(this);
    ShowResults();
    ResetResults();
    m_startTime += m_epochDuration;
    m_endEpochEvent =
        Simulator::Schedule(m_epochDuration, &BurstyAppStatsCalculator::EndEpoch, this);
}

void
BurstyAppStatsCalculator::SetOutputFilename(std::string filename)
{
    m_outputFilename = filename;
}

std::string
BurstyAppStatsCalculator::GetOutputFilename(void) const
{
    return m_outputFilename;
}
