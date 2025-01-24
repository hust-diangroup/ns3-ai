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
 *         Muyuan Shen <muyuan_shen@hust.edu.cn>
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
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(false);
    interface->SetHandleFinish(true);
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        interface->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x01;
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    m_ns3ai_manager_id = msgInterface->GetPy2CppStruct()->managerId;
    msgInterface->CppRecvEnd();
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
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x02;
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    station->m_ns3ai_station_id = msgInterface->GetPy2CppStruct()->stationId;
    msgInterface->CppRecvEnd();

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

    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x03;
    msgInterface->GetCpp2PyStruct()->managerId = m_ns3ai_manager_id;
    msgInterface->GetCpp2PyStruct()->stationId = station->m_ns3ai_station_id;

    NS_ASSERT_MSG(station->m_mcsStats.size() <= 64, "m_mcsStats too long");

    auto& s = msgInterface->GetCpp2PyStruct()->data.stats;
    for (size_t i = 0; i < station->m_mcsStats.size(); i++)
    {
        const WifiMode mode{station->m_mcsStats.at(i).mode};
        s.at(i).nss = station->m_mcsStats.at(i).nss;
        s.at(i).channelWidth = station->m_mcsStats.at(i).channelWidth;
        s.at(i).guardInterval = GetModeGuardInterval(st, mode);
        s.at(i).dataRate =
            mode.GetDataRate(s.at(i).channelWidth, NanoSeconds(s.at(i).guardInterval), s.at(i).nss);
    }
    s[station->m_mcsStats.size()].lastDecay = -1.0;
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    NS_ASSERT_MSG(msgInterface->GetPy2CppStruct()->stationId ==
                      msgInterface->GetCpp2PyStruct()->stationId,
                  "Error 0x03");
    msgInterface->CppRecvEnd();

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
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x05;
    msgInterface->GetCpp2PyStruct()->managerId = m_ns3ai_manager_id;
    msgInterface->GetCpp2PyStruct()->stationId = station->m_ns3ai_station_id;
    msgInterface->GetCpp2PyStruct()->data.decay.decay = m_decay;
    msgInterface->GetCpp2PyStruct()->data.decay.now = Simulator::Now().GetSeconds();
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    NS_ASSERT_MSG(msgInterface->GetPy2CppStruct()->stationId ==
                      msgInterface->GetCpp2PyStruct()->stationId,
                  "Error 0x05");
    msgInterface->CppRecvEnd();
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
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x0a;
    msgInterface->GetCpp2PyStruct()->managerId = m_ns3ai_manager_id;
    msgInterface->GetCpp2PyStruct()->stationId = station->m_ns3ai_station_id;
    msgInterface->GetCpp2PyStruct()->data.decay.decay = m_decay;
    msgInterface->GetCpp2PyStruct()->data.decay.now = Simulator::Now().GetSeconds();
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    NS_ASSERT_MSG(msgInterface->GetPy2CppStruct()->stationId ==
                      msgInterface->GetCpp2PyStruct()->stationId,
                  "Error 0x0a");
    msgInterface->CppRecvEnd();
}

void
AiThompsonSamplingWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                              double ackSnr,
                                              WifiMode ackMode,
                                              double dataSnr,
                                              MHz_u dataChannelWidth,
                                              uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode.GetUniqueName() << dataSnr);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x06;
    msgInterface->GetCpp2PyStruct()->managerId = m_ns3ai_manager_id;
    msgInterface->GetCpp2PyStruct()->stationId = station->m_ns3ai_station_id;
    msgInterface->GetCpp2PyStruct()->data.decay.decay = m_decay;
    msgInterface->GetCpp2PyStruct()->data.decay.now = Simulator::Now().GetSeconds();
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    NS_ASSERT_MSG(msgInterface->GetPy2CppStruct()->stationId ==
                      msgInterface->GetCpp2PyStruct()->stationId,
                  "Error 0x06");
    msgInterface->CppRecvEnd();
}

void
AiThompsonSamplingWifiManager::DoReportAmpduTxStatus(WifiRemoteStation* st,
                                                     uint16_t nSuccessfulMpdus,
                                                     uint16_t nFailedMpdus,
                                                     double rxSnr,
                                                     double dataSnr,
                                                     MHz_u dataChannelWidth,
                                                     uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << nSuccessfulMpdus << nFailedMpdus << rxSnr << dataSnr);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x07;
    msgInterface->GetCpp2PyStruct()->managerId = m_ns3ai_manager_id;
    msgInterface->GetCpp2PyStruct()->stationId = station->m_ns3ai_station_id;
    msgInterface->GetCpp2PyStruct()->var = (uint64_t)nSuccessfulMpdus << 32 | nFailedMpdus;
    msgInterface->GetCpp2PyStruct()->data.decay.decay = m_decay;
    msgInterface->GetCpp2PyStruct()->data.decay.now = Simulator::Now().GetSeconds();
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    NS_ASSERT_MSG(msgInterface->GetPy2CppStruct()->stationId ==
                      msgInterface->GetCpp2PyStruct()->stationId,
                  "Error 0x07");
    msgInterface->CppRecvEnd();
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
        return std::max(GetGuardInterval(st).ToInteger(Time::NS), GetGuardInterval().ToInteger(Time::NS));
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
AiThompsonSamplingWifiManager::DoGetDataTxVector(WifiRemoteStation* st, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << st);
    InitializeStation(st);
    auto station = static_cast<AiThompsonSamplingWifiRemoteStation*>(st);
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x08;
    msgInterface->GetCpp2PyStruct()->managerId = m_ns3ai_manager_id;
    msgInterface->GetCpp2PyStruct()->stationId = station->m_ns3ai_station_id;
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    WifiMode mode = station->m_mcsStats.at(msgInterface->GetPy2CppStruct()->res).mode;
    uint8_t nss = msgInterface->GetPy2CppStruct()->stats.nss;
    uint16_t channelWidth =
        std::min(msgInterface->GetPy2CppStruct()->stats.channelWidth, static_cast<uint16_t>(GetPhy()->GetChannelWidth()));
    uint16_t guardInterval = msgInterface->GetPy2CppStruct()->stats.guardInterval;
    msgInterface->CppRecvEnd();

    uint64_t rate = mode.GetDataRate(channelWidth, NanoSeconds(guardInterval), nss);
    if (m_currentRate != rate)
    {
        NS_LOG_DEBUG("New datarate: " << rate);
        m_currentRate = rate;
    }

    return WifiTxVector(
        mode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(mode.GetModulationClass(), GetShortPreambleEnabled()),
        NanoSeconds(guardInterval),
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
    Ns3AiMsgInterfaceImpl<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()
            ->GetInterface<AiThompsonSamplingEnvStruct, AiThompsonSamplingActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->type = 0x09;
    msgInterface->GetCpp2PyStruct()->managerId = m_ns3ai_manager_id;
    msgInterface->GetCpp2PyStruct()->stationId = station->m_ns3ai_station_id;
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    WifiMode mode = station->m_mcsStats.at(msgInterface->GetPy2CppStruct()->res).mode;
    uint8_t nss = msgInterface->GetPy2CppStruct()->stats.nss;
    uint16_t channelWidth =
        std::min(msgInterface->GetPy2CppStruct()->stats.channelWidth, static_cast<uint16_t>(GetPhy()->GetChannelWidth()));
    uint16_t guardInterval = msgInterface->GetPy2CppStruct()->stats.guardInterval;
    msgInterface->CppRecvEnd();

    // Make sure control frames are sent using 1 spatial stream.
    NS_ASSERT(nss == 1);

    return WifiTxVector(
        mode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(mode.GetModulationClass(), GetShortPreambleEnabled()),
        NanoSeconds(guardInterval),
        GetNumberOfAntennas(),
        nss,
        0, // NESS
        channelWidth,
        GetAggregation(station),
        false);
}

} // namespace ns3
