/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
//
// Copyright (c) 2021 SIGNET Lab, Department of Information Engineering,
// University of Padova
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "trace-file-burst-generator.h"

#include "ns3/csv-reader.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TraceFileBurstGenerator");

NS_OBJECT_ENSURE_REGISTERED(TraceFileBurstGenerator);

TypeId
TraceFileBurstGenerator::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::TraceFileBurstGenerator")
            .SetParent<BurstGenerator>()
            .SetGroupName("Applications")
            .AddConstructor<TraceFileBurstGenerator>()
            .AddAttribute("TraceFile",
                          "The path to the trace file",
                          StringValue(""),
                          MakeStringAccessor(&TraceFileBurstGenerator::GetTraceFile,
                                             &TraceFileBurstGenerator::SetTraceFile),
                          MakeStringChecker())
            .AddAttribute(
                "StartTime",
                "The trace will only generate traced traffic after a start time offset [s]",
                DoubleValue(0.0),
                MakeDoubleAccessor(&TraceFileBurstGenerator::GetStartTime,
                                   &TraceFileBurstGenerator::SetStartTime),
                MakeDoubleChecker<double>(0.0));
    return tid;
}

TraceFileBurstGenerator::TraceFileBurstGenerator()
{
    NS_LOG_FUNCTION(this);
}

TraceFileBurstGenerator::~TraceFileBurstGenerator()
{
    NS_LOG_FUNCTION(this);
}

void
TraceFileBurstGenerator::DoDispose(void)
{
    NS_LOG_FUNCTION(this);

    ClearBurstQueue();

    // chain up
    BurstGenerator::DoDispose();
}

void
TraceFileBurstGenerator::SetStartTime(double startTime)
{
    NS_LOG_FUNCTION(this << startTime);
    if (startTime != m_startTime)
    {
        m_startTime = startTime;
        m_isFinalized = false;
    }
}

double
TraceFileBurstGenerator::GetStartTime(void) const
{
    return m_startTime;
}

void
TraceFileBurstGenerator::SetTraceFile(std::string traceFile)
{
    NS_LOG_FUNCTION(this << traceFile);
    if (traceFile != m_traceFile)
    {
        m_traceFile = traceFile;
        m_isFinalized = false;
    }
}

std::string
TraceFileBurstGenerator::GetTraceFile(void) const
{
    return m_traceFile;
}

double
TraceFileBurstGenerator::GetTraceDuration(void)
{
    if (!m_isFinalized)
    {
        ImportTrace();
    }

    return m_traceDuration;
}

bool
TraceFileBurstGenerator::HasNextBurst(void)
{
    NS_LOG_FUNCTION(this);
    if (!m_isFinalized)
    {
        ImportTrace();
    }

    return !m_burstQueue.empty();
}

std::pair<uint32_t, Time>
TraceFileBurstGenerator::GenerateBurst()
{
    NS_LOG_FUNCTION(this);
    if (!m_isFinalized)
    {
        ImportTrace();
    }

    NS_ABORT_MSG_IF(m_burstQueue.empty(),
                    "All bursts from the trace have already been generated, "
                    "you should have checked if HasNextBurst");

    std::pair<uint32_t, Time> burst = m_burstQueue.front();
    m_burstQueue.pop();
    NS_LOG_DEBUG("Generated std::pair(" << burst.first << ", " << burst.second << "); "
                                        << m_burstQueue.size()
                                        << " more bursts excluding the current one");
    return burst;
}

void
TraceFileBurstGenerator::ClearBurstQueue(void)
{
    NS_LOG_FUNCTION(this);
    std::queue<std::pair<uint32_t, Time>> empty;
    std::swap(m_burstQueue, empty);
}

void
TraceFileBurstGenerator::ImportTrace(void)
{
    NS_LOG_FUNCTION(this);

    // extract trace from file
    CsvReader csv(m_traceFile);

    ClearBurstQueue();
    m_traceDuration = 0;
    double cumulativeStartTime = 0;
    uint32_t burstSize;
    double period;
    while (csv.FetchNextRow())
    {
        // Ignore blank lines
        if (csv.IsBlankRow())
        {
            continue;
        }

        // Expecting burst size and period to next burst
        bool ok = csv.GetValue(0, burstSize);
        ok |= csv.GetValue(1, period);
        NS_ABORT_MSG_IF(!ok,
                        "Something went wrong on line " << csv.RowNumber() << " of file "
                                                        << m_traceFile);
        NS_ABORT_MSG_IF(period < 0,
                        "Period to next burst should be non-negative, instead found: "
                            << period << " on line " << csv.RowNumber());

        // Ignore bursts before m_startTime
        if (cumulativeStartTime >= m_startTime)
        {
            m_burstQueue.push(std::make_pair(burstSize, Seconds(period)));
            m_traceDuration += period;
        }
        cumulativeStartTime += period;
    } // while FetchNextRow

    m_isFinalized = true;
    NS_LOG_INFO("Parsed " << m_burstQueue.size() << " bursts from file " << m_traceFile);
}

} // Namespace ns3
