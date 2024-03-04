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

#include "vr-burst-generator.h"

#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"
#include "ns3/random-variable-stream.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VrBurstGenerator");

NS_OBJECT_ENSURE_REGISTERED(VrBurstGenerator);

TypeId
VrBurstGenerator::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::VrBurstGenerator")
            .SetParent<BurstGenerator>()
            .SetGroupName("Applications")
            .AddConstructor<VrBurstGenerator>()
            .AddAttribute("FrameRate",
                          "The frame rate of the VR application [FPS]. "
                          "Only 30 and 60 FPS are currently supported.",
                          DoubleValue(60),
                          MakeDoubleAccessor(&VrBurstGenerator::SetFrameRate,
                                             &VrBurstGenerator::GetFrameRate),
                          MakeDoubleChecker<double>(0))
            .AddAttribute("TargetDataRate",
                          "The target data rate that the VR application will try to achieve.",
                          DataRateValue(DataRate("20Mbps")),
                          MakeDataRateAccessor(&VrBurstGenerator::SetTargetDataRate,
                                               &VrBurstGenerator::GetTargetDataRate),
                          MakeDataRateChecker())
            .AddAttribute("VrAppName",
                          "The VR application on which the model is based upon. Check the "
                          "documentation for further information.",
                          EnumValue(VrAppName::VirusPopper),
                          MakeEnumAccessor<VrAppName>(&VrBurstGenerator::m_appName),
                          MakeEnumChecker(VrAppName::VirusPopper,
                                          "VirusPopper",
                                          VrAppName::Minecraft,
                                          "Minecraft",
                                          VrAppName::GoogleEarthVrCities,
                                          "GoogleEarthVrCities",
                                          VrAppName::GoogleEarthVrTour,
                                          "GoogleEarthVrTour"));
    return tid;
}

VrBurstGenerator::VrBurstGenerator()
{
    NS_LOG_FUNCTION(this);
}

VrBurstGenerator::~VrBurstGenerator()
{
    NS_LOG_FUNCTION(this);
}

int64_t
VrBurstGenerator::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_periodRv->SetStream(stream);
    m_frameSizeRv->SetStream(stream + 1);
    return 2;
}

void
VrBurstGenerator::DoDispose(void)
{
    NS_LOG_FUNCTION(this);

    m_periodRv = 0;
    m_frameSizeRv = 0;

    // chain up
    BurstGenerator::DoDispose();
}

void
VrBurstGenerator::SetFrameRate(double frameRate)
{
    NS_LOG_FUNCTION(this << frameRate);

    NS_ABORT_MSG_UNLESS(frameRate == 30 || frameRate == 60,
                        "Frame rate must be either 30 or 60 FPS, instead frameRate=" << frameRate);
    m_frameRate = frameRate;

    SetupModel();
}

double
VrBurstGenerator::GetFrameRate(void) const
{
    return m_frameRate;
}

void
VrBurstGenerator::SetTargetDataRate(DataRate targetDataRate)
{
    NS_LOG_FUNCTION(this << targetDataRate);

    NS_ABORT_MSG_IF(targetDataRate.GetBitRate() <= 0,
                    "Target data rate must be positive, instead: " << targetDataRate);
    m_targetDataRate = targetDataRate;

    SetupModel();
}

DataRate
VrBurstGenerator::GetTargetDataRate(void) const
{
    return m_targetDataRate;
}

void
VrBurstGenerator::SetVrAppName(VrBurstGenerator::VrAppName vrAppName)
{
    NS_LOG_FUNCTION(this << vrAppName);

    m_appName = vrAppName;
    SetupModel();
}

VrBurstGenerator::VrAppName
VrBurstGenerator::GetVrAppName(void) const
{
    return m_appName;
}

bool
VrBurstGenerator::HasNextBurst(void)
{
    NS_LOG_FUNCTION(this);
    // this burst generator has no limits on the number of bursts
    return true;
}

std::pair<uint32_t, Time>
VrBurstGenerator::GenerateBurst()
{
    NS_LOG_FUNCTION(this);

    // sample current frame size
    uint32_t frameSize = m_frameSizeRv->GetInteger();

    // sample period before next frame
    Time period = Seconds(m_periodRv->GetValue());
    NS_ABORT_MSG_IF(!period.IsPositive(),
                    "Period must be non-negative, instead found period=" << period.As(Time::S));

    NS_LOG_DEBUG("Frame size: " << frameSize << " B, period: " << period.As(Time::S));
    return std::make_pair(frameSize, period);
}

void
VrBurstGenerator::SetupModel()
{
    NS_LOG_FUNCTION(this);

    double alpha{0};
    double beta{0};
    double gamma{0};
    double delta{0};
    double epsilon{0};

    switch (m_appName)
    {
    case VrAppName::VirusPopper:
        alpha = 0.17843005544386825;
        beta = -0.24033549;
        if (m_frameRate == 60)
        {
            gamma = 0.03720502322046791;
        }
        else if (m_frameRate == 30)
        {
            delta = 0.014333111298430356;
            epsilon = 0.17636808;
        }
        else
        {
            NS_ABORT_MSG("Unexpected frame rate: " << m_frameRate);
        }
        break;

    case VrAppName::Minecraft:
        alpha = 0.18570635904452573;
        beta = -0.18721216;
        if (m_frameRate == 60)
        {
            gamma = 0.07132669841811076;
        }
        else if (m_frameRate == 30)
        {
            delta = 0.024192743507827373;
            epsilon = 0.22666163;
        }
        else
        {
            NS_ABORT_MSG("Unexpected frame rate: " << m_frameRate);
        }
        break;

    case VrAppName::GoogleEarthVrCities:
        alpha = 0.259684566301378;
        beta = -0.25390119;
        if (m_frameRate == 60)
        {
            gamma = 0.034571656202610615;
        }
        else if (m_frameRate == 30)
        {
            delta = 0.008953037116942649;
            epsilon = 0.3119082;
        }
        else
        {
            NS_ABORT_MSG("Unexpected frame rate: " << m_frameRate);
        }
        break;

    case VrAppName::GoogleEarthVrTour:
        alpha = 0.25541435742159037;
        beta = -0.20308171;
        if (m_frameRate == 60)
        {
            gamma = 0.03468230656563422;
        }
        else if (m_frameRate == 30)
        {
            delta = 0.010559650431826953;
            epsilon = 0.27560183;
        }
        else
        {
            NS_ABORT_MSG("Unexpected frame rate: " << m_frameRate);
        }
        break;

    default:
        NS_ABORT_MSG("m_appName was not recognized");
        break;
    }

    double fsAvg = m_targetDataRate.GetBitRate() / 8.0 / m_frameRate; // expected frame size [B]
    double ifiAvg = 1.0 / m_frameRate; // expected inter frame interarrival [s]
    double targetRate_mbps = m_targetDataRate.GetBitRate() / 1e6;

    // Model frame size stats
    double fsDispersion = alpha * std::pow(targetRate_mbps, beta);
    double fsScale = fsAvg * fsDispersion;
    NS_LOG_DEBUG("Frame size: loc=" << fsAvg << ", scale=" << fsScale
                                    << " (dispersion=" << fsDispersion << ")");

    m_frameSizeRv = CreateObjectWithAttributes<LogisticRandomVariable>("Location",
                                                                       DoubleValue(fsAvg),
                                                                       "Scale",
                                                                       DoubleValue(fsScale),
                                                                       "Bound",
                                                                       DoubleValue(fsAvg));

    // Model IFI stats
    double ifiDispersion;
    if (m_frameRate == 60)
    {
        ifiDispersion = gamma;
    }
    else if (m_frameRate == 30)
    {
        ifiDispersion = delta * std::pow(targetRate_mbps, epsilon);
    }
    else
    {
        NS_ABORT_MSG("Unexpected frame rate: " << m_frameRate);
    }
    double ifiScale = ifiAvg * ifiDispersion;
    NS_LOG_DEBUG("IFI: loc=" << ifiAvg << ", scale=" << ifiScale << " (dispersion=" << ifiDispersion
                             << ")");

    m_periodRv = CreateObjectWithAttributes<LogisticRandomVariable>("Location",
                                                                    DoubleValue(ifiAvg),
                                                                    "Scale",
                                                                    DoubleValue(ifiScale),
                                                                    "Bound",
                                                                    DoubleValue(ifiAvg));
}

} // Namespace ns3
