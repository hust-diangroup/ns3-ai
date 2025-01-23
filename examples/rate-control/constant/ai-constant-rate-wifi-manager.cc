/*
 * Copyright (c) 2004,2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modify: Xun Deng <dorence@hust.edu.cn>
 *         Hao Yin <haoyin@uw.edu>
 *         Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#include "ai-constant-rate-wifi-manager.h"

#include <ns3/log.h>
#include <ns3/string.h>
#include <ns3/wifi-phy.h>
#include <ns3/wifi-tx-vector.h>

#define Min(a, b) ((a < b) ? a : b)

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AiConstantRateWifiManager");

NS_OBJECT_ENSURE_REGISTERED(AiConstantRateWifiManager);

TypeId
AiConstantRateWifiManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AiConstantRateWifiManager")
            .SetParent<WifiRemoteStationManager>()
            .SetGroupName("Wifi")
            .AddConstructor<AiConstantRateWifiManager>()
            .AddAttribute("DataMode",
                          "The transmission mode to use for every data packet transmission",
                          StringValue("OfdmRate6Mbps"),
                          MakeWifiModeAccessor(&AiConstantRateWifiManager::m_dataMode),
                          MakeWifiModeChecker())
            .AddAttribute("ControlMode",
                          "The transmission mode to use for every RTS packet transmission.",
                          StringValue("OfdmRate6Mbps"),
                          MakeWifiModeAccessor(&AiConstantRateWifiManager::m_ctlMode),
                          MakeWifiModeChecker());
    return tid;
}

AiConstantRateWifiManager::AiConstantRateWifiManager()
{
    NS_LOG_FUNCTION(this);
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(false);
    interface->SetHandleFinish(true);
}

AiConstantRateWifiManager::~AiConstantRateWifiManager()
{
    NS_LOG_FUNCTION(this);
}

WifiRemoteStation*
AiConstantRateWifiManager::DoCreateStation() const
{
    NS_LOG_FUNCTION(this);
    WifiRemoteStation* station = new WifiRemoteStation();
    return station;
}

void
AiConstantRateWifiManager::DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode)
{
    NS_LOG_FUNCTION(this << station << rxSnr << txMode);
}

void
AiConstantRateWifiManager::DoReportRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiConstantRateWifiManager::DoReportDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiConstantRateWifiManager::DoReportRtsOk(WifiRemoteStation* st,
                                         double ctsSnr,
                                         WifiMode ctsMode,
                                         double rtsSnr)
{
    NS_LOG_FUNCTION(this << st << ctsSnr << ctsMode << rtsSnr);
}

void
AiConstantRateWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                          double ackSnr,
                                          WifiMode ackMode,
                                          double dataSnr,
                                          MHz_u dataChannelWidth,
                                          uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode << dataSnr << dataChannelWidth << +dataNss);
}

void
AiConstantRateWifiManager::DoReportFinalRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiConstantRateWifiManager::DoReportFinalDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

WifiTxVector
AiConstantRateWifiManager::DoGetDataTxVector(WifiRemoteStation* st, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << st);
    Ns3AiMsgInterfaceImpl<AiConstantRateEnvStruct, AiConstantRateActStruct>* msgInterface =
        Ns3AiMsgInterface::Get()->GetInterface<AiConstantRateEnvStruct, AiConstantRateActStruct>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->transmitStreams = GetMaxNumberOfTransmitStreams();
    msgInterface->GetCpp2PyStruct()->supportedStreams = GetNumberOfSupportedStreams(st);
    if (m_dataMode.GetModulationClass() == WIFI_MOD_CLASS_HT)
    {
        msgInterface->GetCpp2PyStruct()->mcs = m_dataMode.GetMcsValue();
    }
    else
    {
        msgInterface->GetCpp2PyStruct()->mcs = 0xffU;
    }
    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    uint8_t nss = msgInterface->GetPy2CppStruct()->nss;
    uint8_t next_mcs = msgInterface->GetPy2CppStruct()->next_mcs;
    NS_LOG_FUNCTION(next_mcs);
    msgInterface->CppRecvEnd();

    // uncomment to specify arbitrary MCS
    // m_dataMode = GetMcsSupported (st, next_mcs);

    return WifiTxVector(
        m_dataMode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(m_dataMode.GetModulationClass(), GetShortPreambleEnabled()),
        GetGuardInterval(st),
        GetNumberOfAntennas(),
        nss,
        0,
        GetPhy()->GetTxBandwidth(m_dataMode, std::min(allowedWidth, GetChannelWidth(st))),
        GetAggregation(st));
}

WifiTxVector
AiConstantRateWifiManager::DoGetRtsTxVector(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    return WifiTxVector(
        m_ctlMode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(m_ctlMode.GetModulationClass(), GetShortPreambleEnabled()),
        GetGuardInterval(st),
        1,
        1,
        0,
        GetPhy()->GetTxBandwidth(m_dataMode, GetChannelWidth(st)),
        GetAggregation(st));
}

} // namespace ns3
