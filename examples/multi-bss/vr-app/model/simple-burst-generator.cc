/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "simple-burst-generator.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SimpleBurstGenerator");

NS_OBJECT_ENSURE_REGISTERED(SimpleBurstGenerator);

TypeId
SimpleBurstGenerator::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::SimpleBurstGenerator")
            .SetParent<BurstGenerator>()
            .SetGroupName("Applications")
            .AddConstructor<SimpleBurstGenerator>()
            .AddAttribute("PeriodRv",
                          "A RandomVariableStream used to pick the duration of the period [s].",
                          StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                          MakePointerAccessor(&SimpleBurstGenerator::m_periodRv),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("BurstSizeRv",
                          "A RandomVariableStream used to pick the size of the burst [B].",
                          StringValue("ns3::ConstantRandomVariable[Constant=1e6]"),
                          MakePointerAccessor(&SimpleBurstGenerator::m_burstSizeRv),
                          MakePointerChecker<RandomVariableStream>());
    return tid;
}

SimpleBurstGenerator::SimpleBurstGenerator()
{
    NS_LOG_FUNCTION(this);
}

SimpleBurstGenerator::~SimpleBurstGenerator()
{
    NS_LOG_FUNCTION(this);
}

int64_t
SimpleBurstGenerator::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_periodRv->SetStream(stream);
    m_burstSizeRv->SetStream(stream + 1);
    return 2;
}

void
SimpleBurstGenerator::DoDispose(void)
{
    NS_LOG_FUNCTION(this);

    m_periodRv = 0;
    m_burstSizeRv = 0;

    // chain up
    BurstGenerator::DoDispose();
}

bool
SimpleBurstGenerator::HasNextBurst(void)
{
    NS_LOG_FUNCTION(this);
    // this burst generator has no limits on the number of bursts
    return true;
}

std::pair<uint32_t, Time>
SimpleBurstGenerator::GenerateBurst()
{
    NS_LOG_FUNCTION(this);

    // sample current burst size
    // NOTE: limited to 4 GB per burst by GetInteger
    uint32_t burstSize = m_burstSizeRv->GetInteger();

    // sample period before next burst
    Time period = Seconds(m_periodRv->GetValue());
    NS_ABORT_MSG_IF(!period.IsPositive(),
                    "Period must be non-negative, instead found period=" << period.As(Time::S));

    NS_LOG_DEBUG("Burst size: " << burstSize << " B, period: " << period.As(Time::S));
    return std::make_pair(burstSize, period);
}

} // Namespace ns3
