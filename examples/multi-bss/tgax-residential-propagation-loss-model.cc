/*
 * Copyright (c) 2023 University of Washington
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
 */

#include "tgax-residential-propagation-loss-model.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/pointer.h"
#include <ns3/mobility-building-info.h>

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TgaxResidentialPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(TgaxResidentialPropagationLossModel);

TypeId
TgaxResidentialPropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TgaxResidentialPropagationLossModel")
            .SetParent<PropagationLossModel>()
            .SetGroupName("Wifi")
            .AddConstructor<TgaxResidentialPropagationLossModel>()
            .AddAttribute("Frequency",
                          "The carrier frequency (in Hz) at which propagation occurs",
                          DoubleValue(2.437e9),
                          MakeDoubleAccessor(&TgaxResidentialPropagationLossModel::m_frequencyHz),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "ShadowSigma",
                "Standard deviation (dB) of the normal distribution used to calculate shadowing "
                "loss",
                DoubleValue(5.0),
                MakeDoubleAccessor(&TgaxResidentialPropagationLossModel::m_shadowingSigma),
                MakeDoubleChecker<double>());
    return tid;
}

TgaxResidentialPropagationLossModel::TgaxResidentialPropagationLossModel()
{
    m_shadowingRandomVariable = CreateObject<NormalRandomVariable>();
}

double
TgaxResidentialPropagationLossModel::GetRxPower(double txPowerDbm,
                                                Ptr<MobilityModel> a,
                                                Ptr<MobilityModel> b) const
{
    double distance = a->GetDistanceFrom(b);

    if (distance == 0)
    {
        return txPowerDbm;
    }

    distance = std::max(1.0, distance); // 1m minimum distance
    //
    // Based on the IEEE 802.11-14/0980r6 document, 1 - Residential Scenario
    //
    //     PL(d) = 40.05 + 20 * log10(fc/2.4) + 20 * log10(min(d,5)) +
    //             18.3 * (d/floors)^(((d/floors)+2)/((d/floors)+1) - 0.46) + 5 * (d/walls)
    //
    //  if d>5 then
    //     PL(d) += 35 * log10(d/5)
    //

    double pathlossDb;
    //    double shadowingDb = 0;
    double breakpointDistance = 5; // meters
    double fc = 2.4e9;             // carrier frequency, Hz
    uint16_t floors = 0;
    uint16_t walls = 0;
    Ptr<MobilityBuildingInfo> aInfo = a->GetObject<MobilityBuildingInfo>();
    Ptr<MobilityBuildingInfo> bInfo = b->GetObject<MobilityBuildingInfo>();
    if (aInfo && bInfo)
    {
        if (!aInfo->IsIndoor() || !bInfo->IsIndoor())
        {
            NS_LOG_DEBUG("One or both nodes is outdoor, so returning zero signal power");
            return 0;
        }
        floors = std::abs(aInfo->GetFloorNumber() - bInfo->GetFloorNumber());
        walls = std::abs(aInfo->GetRoomNumberX() - bInfo->GetRoomNumberX()) +
                std::abs(aInfo->GetRoomNumberY() - bInfo->GetRoomNumberY());
    }

    pathlossDb = 40.05 + 20 * std::log10(m_frequencyHz / fc) +
                 20 * std::log10(std::min(distance, breakpointDistance));
    if (distance > breakpointDistance)
    {
        pathlossDb += 35 * std::log10(distance / 5);
    }
    if (floors)
    {
        pathlossDb +=
            18.3 * std::pow((distance / floors),
                            ((distance / floors) + 2.0) / ((distance / floors) + 1.0) - 0.46);
    }
    if (walls)
    {
        pathlossDb += 5.0 * (walls); // Changed (distance/walls) to only (walls) because the
                                     // pathloss would isolate the rooms
    }

    // TODO:  cache the shadowingDb value and reuse until positions change
    //    shadowingDb = m_shadowingRandomVariable->GetValue(
    //        0,
    //        m_shadowingSigma * m_shadowingSigma); // Disabled shadowing because nodes do not move
    // std::cout << "Distance " << distance << " Pathloss " << pathlossDb << " Floor " << floors
    //           << " walls " << walls << std::endl;
    return txPowerDbm - pathlossDb;
    // -shadowingDb;
}

double
TgaxResidentialPropagationLossModel::DoCalcRxPower(double txPowerDbm,
                                                   Ptr<MobilityModel> a,
                                                   Ptr<MobilityModel> b) const
{
    double distance = a->GetDistanceFrom(b);

    if (distance == 0)
    {
        return txPowerDbm;
    }

    distance = std::max(1.0, distance); // 1m minimum distance
    //
    // Based on the IEEE 802.11-14/0980r6 document, 1 - Residential Scenario
    //
    //     PL(d) = 40.05 + 20 * log10(fc/2.4) + 20 * log10(min(d,5)) +
    //             18.3 * (d/floors)^(((d/floors)+2)/((d/floors)+1) - 0.46) + 5 * (d/walls)
    //
    //  if d>5 then
    //     PL(d) += 35 * log10(d/5)
    //

    double pathlossDb;
    //    double shadowingDb = 0;
    double breakpointDistance = 5; // meters
    double fc = 2.4e9;             // carrier frequency, Hz
    uint16_t floors = 0;
    uint16_t walls = 0;
    Ptr<MobilityBuildingInfo> aInfo = a->GetObject<MobilityBuildingInfo>();
    Ptr<MobilityBuildingInfo> bInfo = b->GetObject<MobilityBuildingInfo>();
    if (aInfo && bInfo)
    {
        if (!aInfo->IsIndoor() || !bInfo->IsIndoor())
        {
            NS_LOG_DEBUG("One or both nodes is outdoor, so returning zero signal power");
            return 0;
        }
        floors = std::abs(aInfo->GetFloorNumber() - bInfo->GetFloorNumber());
        walls = std::abs(aInfo->GetRoomNumberX() - bInfo->GetRoomNumberX()) +
                std::abs(aInfo->GetRoomNumberY() - bInfo->GetRoomNumberY());
    }

    pathlossDb = 40.05 + 20 * std::log10(m_frequencyHz / fc) +
                 20 * std::log10(std::min(distance, breakpointDistance));
    if (distance > breakpointDistance)
    {
        pathlossDb += 35 * std::log10(distance / 5);
    }
    if (floors)
    {
        pathlossDb +=
            18.3 * std::pow((distance / floors),
                            ((distance / floors) + 2.0) / ((distance / floors) + 1.0) - 0.46);
    }
    if (walls)
    {
        pathlossDb += 5.0 * (walls); // Changed (distance/walls) to only (walls) because the
                                     // pathloss would isolate the rooms
    }

    // TODO:  cache the shadowingDb value and reuse until positions change
    //    shadowingDb = m_shadowingRandomVariable->GetValue(0, m_shadowingSigma * m_shadowingSigma);
    //    // std::cout << "Distance " << distance << " Pathloss " << pathlossDb << " Floor " <<
    //    floors
    //    //           << " walls " << walls << std::endl;
    return txPowerDbm - pathlossDb;
}

int64_t
TgaxResidentialPropagationLossModel::DoAssignStreams(int64_t stream)
{
    m_shadowingRandomVariable->SetStream(stream);
    return 1;
}

} // namespace ns3
