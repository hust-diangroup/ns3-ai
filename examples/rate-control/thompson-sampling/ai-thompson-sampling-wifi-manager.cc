/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 IITP RAS
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
 * Author: Alexander Krotov <krotov@iitp.ru>
 * Modify: Xun Deng <dorence@hust.edu.cn>
 *         Hao Yin <haoyin@uw.edu>
 */

#include "ai-thompson-sampling-wifi-manager.h"

#include <ns3/core-module.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/wifi-phy.h>

#include <iostream>
#include <vector>

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(AiThompsonSamplingWifiManager);

NS_LOG_COMPONENT_DEFINE("AiThompsonSamplingWifiManager");

Ns3AiRl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct> m_ns3ai_mod =
    Ns3AiRl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>(4096, false);

/**
 * A structure containing parameters of a single rate and its
 * statistics.
 */
struct AiRateStats
{
    WifiMode mode;         ///< MCS
    uint16_t channelWidth; ///< channel width in MHz
    uint8_t nss;           ///< Number of spatial streams
};

/**
 * Holds station state and collected statistics.
 *
 * This struct extends from WifiRemoteStation to hold additional
 * information required by ThompsonSamplingWifiManager.
 */
struct AiThompsonSamplingWifiRemoteStation : public WifiRemoteStation
{
    int8_t m_ns3ai_station_id;
    std::vector<AiRateStats> m_mcsStats; //!< Collected statistics
};

TypeId
AiThompsonSamplingWifiManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AiThompsonSamplingWifiManager")
            .SetParent<WifiRemoteStationManager>()
            .SetGroupName("Wifi")
            .AddConstructor<AiThompsonSamplingWifiManager>()
            .AddAttribute(
                "Decay",
                "Exponential decay coefficient, Hz; zero is a valid value for static scenarios",
                DoubleValue(1.0),
                MakeDoubleAccessor(&AiThompsonSamplingWifiManager::m_decay),
                MakeDoubleChecker<double>(0.0))
            .AddTraceSource("Rate",
                            "Traced value for rate changes (b/s)",
                            MakeTraceSourceAccessor(&AiThompsonSamplingWifiManager::m_currentRate),
                            "ns3::TracedValueCallback::Uint64");
    return tid;
}

AiThompsonSamplingWifiManager::AiThompsonSamplingWifiManager()
    : m_currentRate{0}
{
    NS_LOG_FUNCTION(this);

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x01;
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    m_ns3ai_manager_id = m_ns3ai_mod.m_single_act->managerId;
    m_ns3ai_mod.get_act_end();
}

AiThompsonSamplingWifiManager::~AiThompsonSamplingWifiManager()
{
    NS_LOG_FUNCTION(this);
}

WifiRemoteStation*
AiThompsonSamplingWifiManager::DoCreateStation() const
{
    NS_LOG_FUNCTION(this);
    AiThompsonSamplingWifiRemoteStation* station = new AiThompsonSamplingWifiRemoteStation();

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x02;
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    station->m_ns3ai_station_id = m_ns3ai_mod.m_single_act->stationId;
    m_ns3ai_mod.get_act_end();

    return station;
}

void
AiThompsonSamplingWifiManager::InitializeStation(WifiRemoteStation* st) const
{
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);
    if (!station->m_mcsStats.empty())
    {
        return;
    }

    // Add HT, VHT or HE MCSes
    for (const auto& mode : GetPhy()->GetMcsList())
    {
        for (uint16_t j = 20; j <= GetPhy()->GetChannelWidth(); j *= 2)
        {
            WifiModulationClass modulationClass = WIFI_MOD_CLASS_HT;
            if (GetVhtSupported())
            {
                modulationClass = WIFI_MOD_CLASS_VHT;
            }
            if (GetHeSupported())
            {
                modulationClass = WIFI_MOD_CLASS_HE;
            }
            if (mode.GetModulationClass() == modulationClass)
            {
                for (uint8_t k = 1; k <= GetPhy()->GetMaxSupportedTxSpatialStreams(); k++)
                {
                    if (mode.IsAllowed(j, k))
                    {
                        AiRateStats stats;
                        stats.mode = mode;
                        stats.channelWidth = j;
                        stats.nss = k;
                        station->m_mcsStats.push_back(stats);
                    }
                }
            }
        }
    }

    if (station->m_mcsStats.empty())
    {
        // Add legacy non-HT modes.
        for (uint8_t i = 0; i < GetNSupported(station); i++)
        {
            AiRateStats stats;
            stats.mode = GetSupported(station, i);
            if (stats.mode.GetModulationClass() == WIFI_MOD_CLASS_DSSS ||
                stats.mode.GetModulationClass() == WIFI_MOD_CLASS_HR_DSSS)
            {
                stats.channelWidth = 22;
            }
            else
            {
                stats.channelWidth = 20;
            }
            stats.nss = 1;
            station->m_mcsStats.push_back(stats);
        }
    }

    NS_ASSERT_MSG(!station->m_mcsStats.empty(), "No usable MCS found");

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x03;
    m_ns3ai_mod.m_single_env->managerId = m_ns3ai_manager_id;
    m_ns3ai_mod.m_single_env->stationId = station->m_ns3ai_station_id;

    NS_ASSERT_MSG(station->m_mcsStats.size() <= 64, "m_mcsStats too long");

    auto& s = m_ns3ai_mod.m_single_env->data.stats;
    for (size_t i = 0; i < station->m_mcsStats.size(); i++)
    {
        const WifiMode mode{station->m_mcsStats.at(i).mode};
        s.at(i).nss = station->m_mcsStats.at(i).nss;
        s.at(i).channelWidth = station->m_mcsStats.at(i).channelWidth;
        s.at(i).guardInterval = GetModeGuardInterval(st, mode);
        s.at(i).dataRate =
            mode.GetDataRate(s.at(i).channelWidth, s.at(i).guardInterval, s.at(i).nss);
    }
    s[station->m_mcsStats.size()].lastDecay = -1.0;
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    NS_ASSERT_MSG(m_ns3ai_mod.m_single_act->stationId == m_ns3ai_mod.m_single_env->stationId,
                  "Error 0x03");
    m_ns3ai_mod.get_act_end();

    UpdateNextMode(st);
}

void
AiThompsonSamplingWifiManager::DoReportRxOk(WifiRemoteStation* station,
                                            double rxSnr,
                                            WifiMode txMode)
{
    NS_LOG_FUNCTION(this << station << rxSnr << txMode);
}

void
AiThompsonSamplingWifiManager::DoReportRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiThompsonSamplingWifiManager::DoReportDataFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x05;
    m_ns3ai_mod.m_single_env->managerId = m_ns3ai_manager_id;
    m_ns3ai_mod.m_single_env->stationId = station->m_ns3ai_station_id;
    m_ns3ai_mod.m_single_env->data.decay.decay = m_decay;
    m_ns3ai_mod.m_single_env->data.decay.now = Simulator::Now().GetSeconds();
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    NS_ASSERT_MSG(m_ns3ai_mod.m_single_act->stationId == m_ns3ai_mod.m_single_env->stationId,
                  "Error 0x05");
    m_ns3ai_mod.get_act_end();
}

void
AiThompsonSamplingWifiManager::DoReportRtsOk(WifiRemoteStation* st,
                                             double ctsSnr,
                                             WifiMode ctsMode,
                                             double rtsSnr)
{
    NS_LOG_FUNCTION(this << st << ctsSnr << ctsMode.GetUniqueName() << rtsSnr);
}

void
AiThompsonSamplingWifiManager::UpdateNextMode(WifiRemoteStation* st) const
{
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);
    NS_ASSERT(!station->m_mcsStats.empty());

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x0a;
    m_ns3ai_mod.m_single_env->managerId = m_ns3ai_manager_id;
    m_ns3ai_mod.m_single_env->stationId = station->m_ns3ai_station_id;
    m_ns3ai_mod.m_single_env->data.decay.decay = m_decay;
    m_ns3ai_mod.m_single_env->data.decay.now = Simulator::Now().GetSeconds();
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    NS_ASSERT_MSG(m_ns3ai_mod.m_single_act->stationId == m_ns3ai_mod.m_single_env->stationId,
                  "Error 0x0a");
    m_ns3ai_mod.get_act_end();
}

void
AiThompsonSamplingWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                              double ackSnr,
                                              WifiMode ackMode,
                                              double dataSnr,
                                              uint16_t dataChannelWidth,
                                              uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode.GetUniqueName() << dataSnr);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x06;
    m_ns3ai_mod.m_single_env->managerId = m_ns3ai_manager_id;
    m_ns3ai_mod.m_single_env->stationId = station->m_ns3ai_station_id;
    m_ns3ai_mod.m_single_env->data.decay.decay = m_decay;
    m_ns3ai_mod.m_single_env->data.decay.now = Simulator::Now().GetSeconds();
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    NS_ASSERT_MSG(m_ns3ai_mod.m_single_act->stationId == m_ns3ai_mod.m_single_env->stationId,
                  "Error 0x06");
    m_ns3ai_mod.get_act_end();
}

void
AiThompsonSamplingWifiManager::DoReportAmpduTxStatus(WifiRemoteStation* st,
                                                     uint16_t nSuccessfulMpdus,
                                                     uint16_t nFailedMpdus,
                                                     double rxSnr,
                                                     double dataSnr,
                                                     uint16_t dataChannelWidth,
                                                     uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << nSuccessfulMpdus << nFailedMpdus << rxSnr << dataSnr);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x07;
    m_ns3ai_mod.m_single_env->managerId = m_ns3ai_manager_id;
    m_ns3ai_mod.m_single_env->stationId = station->m_ns3ai_station_id;
    m_ns3ai_mod.m_single_env->var = (uint64_t)nSuccessfulMpdus << 32 | nFailedMpdus;
    m_ns3ai_mod.m_single_env->data.decay.decay = m_decay;
    m_ns3ai_mod.m_single_env->data.decay.now = Simulator::Now().GetSeconds();
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    NS_ASSERT_MSG(m_ns3ai_mod.m_single_act->stationId == m_ns3ai_mod.m_single_env->stationId,
                  "Error 0x07");
    m_ns3ai_mod.get_act_end();
}

void
AiThompsonSamplingWifiManager::DoReportFinalRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiThompsonSamplingWifiManager::DoReportFinalDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

uint16_t
AiThompsonSamplingWifiManager::GetModeGuardInterval(WifiRemoteStation* st, WifiMode mode) const
{
    if (mode.GetModulationClass() == WIFI_MOD_CLASS_HE)
    {
        return std::max(GetGuardInterval(st), GetGuardInterval());
    }
    else if ((mode.GetModulationClass() == WIFI_MOD_CLASS_HT) ||
             (mode.GetModulationClass() == WIFI_MOD_CLASS_VHT))
    {
        return std::max<uint16_t>(GetShortGuardIntervalSupported(st) ? 400 : 800,
                                  GetShortGuardIntervalSupported() ? 400 : 800);
    }
    else
    {
        return 800;
    }
}

WifiTxVector
AiThompsonSamplingWifiManager::DoGetDataTxVector(WifiRemoteStation* st, uint16_t allowedWidth)
{
    NS_LOG_FUNCTION(this << st);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x08;
    m_ns3ai_mod.m_single_env->managerId = m_ns3ai_manager_id;
    m_ns3ai_mod.m_single_env->stationId = station->m_ns3ai_station_id;
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    WifiMode mode = station->m_mcsStats.at(m_ns3ai_mod.m_single_act->res).mode;
    uint8_t nss = m_ns3ai_mod.m_single_act->stats.nss;
    uint16_t channelWidth =
        std::min(m_ns3ai_mod.m_single_act->stats.channelWidth, GetPhy()->GetChannelWidth());
    uint16_t guardInterval = m_ns3ai_mod.m_single_act->stats.guardInterval;
    m_ns3ai_mod.get_act_end();

    uint64_t rate = mode.GetDataRate(channelWidth, guardInterval, nss);
    if (m_currentRate != rate)
    {
        NS_LOG_DEBUG("New datarate: " << rate);
        m_currentRate = rate;
    }

    return WifiTxVector(
        mode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(mode.GetModulationClass(), GetShortPreambleEnabled()),
        guardInterval,
        GetNumberOfAntennas(),
        nss,
        0, // NESS
        GetPhy()->GetTxBandwidth(mode, GetChannelWidth(st)),
        GetAggregation(station),
        false);
}

WifiTxVector
AiThompsonSamplingWifiManager::DoGetRtsTxVector(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_single_env->type = 0x09;
    m_ns3ai_mod.m_single_env->managerId = m_ns3ai_manager_id;
    m_ns3ai_mod.m_single_env->stationId = station->m_ns3ai_station_id;
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    WifiMode mode = station->m_mcsStats.at(m_ns3ai_mod.m_single_act->res).mode;
    uint8_t nss = m_ns3ai_mod.m_single_act->stats.nss;
    uint16_t channelWidth =
        std::min(m_ns3ai_mod.m_single_act->stats.channelWidth, GetPhy()->GetChannelWidth());
    uint16_t guardInterval = m_ns3ai_mod.m_single_act->stats.guardInterval;
    m_ns3ai_mod.get_act_end();

    // Make sure control frames are sent using 1 spatial stream.
    NS_ASSERT(nss == 1);

    return WifiTxVector(
        mode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(mode.GetModulationClass(), GetShortPreambleEnabled()),
        guardInterval,
        GetNumberOfAntennas(),
        nss,
        0, // NESS
        channelWidth,
        GetAggregation(station),
        false);
}

} // namespace ns3
