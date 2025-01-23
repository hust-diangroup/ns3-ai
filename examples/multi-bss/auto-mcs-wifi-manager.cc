/*
 * Copyright (c) 2006 INRIA
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
 */

#include "auto-mcs-wifi-manager.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/wifi-phy.h"

#include <algorithm>
#include <absl/time/time.h>

namespace ns3
{

/**
 * \brief hold per-remote-station state for Auto Mcs Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the Auto Mcs Wifi manager
 */
struct AutoMcsWifiRemoteStation : public WifiRemoteStation
{
    double m_lastSnrObserved; //!< SNR of most recently reported packet sent to the remote station
    uint16_t m_lastChannelWidthObserved; //!< Channel width (in MHz) of most recently reported
                                         //!< packet sent to the remote station
    uint16_t m_lastNssObserved; //!<  Number of spatial streams of most recently reported packet
                                //!<  sent to the remote station
    double m_lastSnrCached;     //!< SNR most recently used to select a rate
    uint8_t m_lastNss;   //!< Number of spatial streams most recently used to the remote station
    WifiMode m_lastMode; //!< Mode most recently used to the remote station
    uint16_t
        m_lastChannelWidth; //!< Channel width (in MHz) most recently used to the remote station
};

/// To avoid using the cache before a valid value has been cached
static const double CACHE_INITIAL_VALUE = -100;

NS_OBJECT_ENSURE_REGISTERED(AutoMcsWifiManager);

NS_LOG_COMPONENT_DEFINE("AutoMcsWifiManager");

TypeId
AutoMcsWifiManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AutoMcsWifiManager")
            .SetParent<WifiRemoteStationManager>()
            .SetGroupName("Wifi")
            .AddConstructor<AutoMcsWifiManager>()
            .AddAttribute("BerThreshold",
                          "The maximum Bit Error Rate acceptable at any transmission mode",
                          DoubleValue(1e-7), // This default value was modified
                          MakeDoubleAccessor(&AutoMcsWifiManager::m_ber),
                          MakeDoubleChecker<double>())
            .AddAttribute("autoMCS",
                          "If enabled, select the best MCS for each STA-AP pair given the SNR.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AutoMcsWifiManager::m_autoMCS),
                          MakeBooleanChecker())
            .AddTraceSource("Rate",
                            "Traced value for rate changes (b/s)",
                            MakeTraceSourceAccessor(&AutoMcsWifiManager::m_currentRate),
                            "ns3::TracedValueCallback::Uint64");
    return tid;
}

AutoMcsWifiManager::AutoMcsWifiManager()
    : m_currentRate(0)
{
    NS_LOG_FUNCTION(this);
}

AutoMcsWifiManager::~AutoMcsWifiManager()
{
    NS_LOG_FUNCTION(this);
}

void
AutoMcsWifiManager::SetupPhy(const Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    WifiRemoteStationManager::SetupPhy(phy);
}

uint16_t
AutoMcsWifiManager::GetChannelWidthForNonHtMode(WifiMode mode) const
{
    NS_ASSERT(mode.GetModulationClass() != WIFI_MOD_CLASS_HT &&
              mode.GetModulationClass() != WIFI_MOD_CLASS_VHT &&
              mode.GetModulationClass() != WIFI_MOD_CLASS_HE);
    if (mode.GetModulationClass() == WIFI_MOD_CLASS_DSSS ||
        mode.GetModulationClass() == WIFI_MOD_CLASS_HR_DSSS)
    {
        return 22;
    }
    else
    {
        return 20;
    }
}

void
AutoMcsWifiManager::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    BuildSnrThresholds();
}

// void
// IdealWifiManager::PrintOutPer(double snr, WifiTxVector txVector)
// {
// }

void
AutoMcsWifiManager::BuildSnrThresholds()
{
    m_thresholds.clear();
    WifiTxVector txVector;
    uint8_t nss = 1;
    for (const auto& mode : GetPhy()->GetModeList())
    {
        txVector.SetChannelWidth(GetChannelWidthForNonHtMode(mode));
        txVector.SetNss(nss);
        txVector.SetMode(mode);
        NS_LOG_DEBUG("Adding mode = " << mode.GetUniqueName());
        AddSnrThreshold(txVector, GetPhy()->CalculateSnr(txVector, m_ber));
    }
    // Add all MCSes
    if (GetHtSupported())
    {
        for (const auto& mode : GetPhy()->GetMcsList())
        {
            for (uint16_t j = 20; j <= GetPhy()->GetChannelWidth(); j *= 2)
            {
                txVector.SetChannelWidth(j);
                if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT)
                {
                    uint16_t guardInterval = GetShortGuardIntervalSupported() ? 400 : 800;
                    txVector.SetGuardInterval(NanoSeconds(guardInterval));
                    // derive NSS from the MCS index
                    nss = (mode.GetMcsValue() / 8) + 1;
                    NS_LOG_DEBUG("Adding mode = " << mode.GetUniqueName() << " channel width " << j
                                                  << " nss " << +nss << " GI " << guardInterval);
                    txVector.SetNss(nss);
                    txVector.SetMode(mode);
                    AddSnrThreshold(txVector, GetPhy()->CalculateSnr(txVector, m_ber));
                }
                else // VHT or HE
                {
                    uint16_t guardInterval;
                    if (mode.GetModulationClass() == WIFI_MOD_CLASS_VHT)
                    {
                        guardInterval = GetShortGuardIntervalSupported() ? 400 : 800;
                    }
                    else
                    {
                        guardInterval = GetGuardInterval().ToInteger(Time::NS);
                    }
                    txVector.SetGuardInterval(NanoSeconds(guardInterval));
                    for (uint8_t k = 1; k <= GetPhy()->GetMaxSupportedTxSpatialStreams(); k++)
                    {
                        if (mode.IsAllowed(j, k))
                        {
                            NS_LOG_DEBUG("Adding mode = " << mode.GetUniqueName()
                                                          << " channel width " << j << " nss " << +k
                                                          << " GI " << guardInterval);
                            txVector.SetNss(k);
                            txVector.SetMode(mode);
                            AddSnrThreshold(txVector, GetPhy()->CalculateSnr(txVector, m_ber));
                            std::vector<double> berThreshold{8.37e-7};
                            // std::vector<double> berThreshold{1e-6, 1e-7, 1e-8};

                            // std::vector<double> berThreshold{1.0 - std::pow((1.0 - 0.1),
                            // 12000.0),
                            //                                  1.0 - std::pow((1.0 - 0.01),
                            //                                  12000.0), 1.0 -
                            //                                      std::pow((1.0 - 0.001),
                            //                                      12000.0)};
                            // for (auto ber : berThreshold)
                            // {
                            //     std::cout << "Mode " << mode << " Requires an SNR of  "
                            //               << RatioToDb(GetPhy()->CalculateSnr(txVector, ber))
                            //               << " for a ber " << ber << std::endl;
                            // }
                        }
                        else
                        {
                            NS_LOG_DEBUG("Mode = " << mode.GetUniqueName() << " disallowed");
                        }
                    }
                }
            }
        }
    }
}

double
AutoMcsWifiManager::GetSnrThreshold(WifiTxVector txVector)
{
    NS_LOG_FUNCTION(this << txVector);
    auto it = std::find_if(m_thresholds.begin(),
                           m_thresholds.end(),
                           [&txVector](const std::pair<double, WifiTxVector>& p) -> bool {
                               return ((txVector.GetMode() == p.second.GetMode()) &&
                                       (txVector.GetNss() == p.second.GetNss()) &&
                                       (txVector.GetChannelWidth() == p.second.GetChannelWidth()));
                           });
    if (it == m_thresholds.end())
    {
        // This means capabilities have changed in runtime, hence rebuild SNR thresholds
        BuildSnrThresholds();
        it = std::find_if(m_thresholds.begin(),
                          m_thresholds.end(),
                          [&txVector](const std::pair<double, WifiTxVector>& p) -> bool {
                              return ((txVector.GetMode() == p.second.GetMode()) &&
                                      (txVector.GetNss() == p.second.GetNss()) &&
                                      (txVector.GetChannelWidth() == p.second.GetChannelWidth()));
                          });
        NS_ASSERT_MSG(it != m_thresholds.end(), "SNR threshold not found");
    }
    return it->first;
}

void
AutoMcsWifiManager::AddSnrThreshold(WifiTxVector txVector, double snr)
{
    NS_LOG_FUNCTION(this << txVector.GetMode().GetUniqueName() << txVector.GetChannelWidth()
                         << snr);
    m_thresholds.emplace_back(snr, txVector);
}

WifiRemoteStation*
AutoMcsWifiManager::DoCreateStation() const
{
    NS_LOG_FUNCTION(this);
    AutoMcsWifiRemoteStation* station = new AutoMcsWifiRemoteStation();
    Reset(station);
    return station;
}

void
AutoMcsWifiManager::Reset(WifiRemoteStation* station) const
{
    NS_LOG_FUNCTION(this << station);
    AutoMcsWifiRemoteStation* st = static_cast<AutoMcsWifiRemoteStation*>(station);
    st->m_lastSnrObserved = 0.0;
    st->m_lastChannelWidthObserved = 0;
    st->m_lastNssObserved = 1;
    st->m_lastSnrCached = CACHE_INITIAL_VALUE;
    st->m_lastMode = GetDefaultMode();
    st->m_lastChannelWidth = 0;
    st->m_lastNss = 1;
}

void
AutoMcsWifiManager::DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode)
{
    NS_LOG_FUNCTION(this << station << rxSnr << txMode);
    // choosenMCS.emplace_back(txMode.GetMcsValue());
}

void
AutoMcsWifiManager::DoReportRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AutoMcsWifiManager::DoReportDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AutoMcsWifiManager::DoReportRtsOk(WifiRemoteStation* st,
                                  double ctsSnr,
                                  WifiMode ctsMode,
                                  double rtsSnr)
{
    NS_LOG_FUNCTION(this << st << ctsSnr << ctsMode.GetUniqueName() << rtsSnr);
    AutoMcsWifiRemoteStation* station = static_cast<AutoMcsWifiRemoteStation*>(st);
    station->m_lastSnrObserved = rtsSnr;
    station->m_lastChannelWidthObserved =
        GetPhy()->GetChannelWidth() >= 40 ? 20 : GetPhy()->GetChannelWidth();
    station->m_lastNssObserved = 1;
}

void
AutoMcsWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                   double ackSnr,
                                   WifiMode ackMode,
                                   double dataSnr,
                                   MHz_u dataChannelWidth,
                                   uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode.GetUniqueName() << dataSnr << dataChannelWidth
                         << +dataNss);
    AutoMcsWifiRemoteStation* station = static_cast<AutoMcsWifiRemoteStation*>(st);
    if (dataSnr == 0)
    {
        NS_LOG_WARN("DataSnr reported to be zero; not saving this report.");
        return;
    }
    station->m_lastSnrObserved = dataSnr;
    station->m_lastChannelWidthObserved = dataChannelWidth;
    station->m_lastNssObserved = dataNss;
    // The default value of m_lastMode is non-HT, thus invalid for GetMcsValue
    if (station->m_lastMode != GetDefaultMode())
    {
        choosenMCS.emplace_back(station->m_lastMode.GetMcsValue());
    }
}

void
AutoMcsWifiManager::DoReportAmpduTxStatus(WifiRemoteStation* st,
                                          uint16_t nSuccessfulMpdus,
                                          uint16_t nFailedMpdus,
                                          double rxSnr,
                                          double dataSnr,
                                          MHz_u dataChannelWidth,
                                          uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << nSuccessfulMpdus << nFailedMpdus << rxSnr << dataSnr
                         << dataChannelWidth << +dataNss);
    AutoMcsWifiRemoteStation* station = static_cast<AutoMcsWifiRemoteStation*>(st);
    if (dataSnr == 0)
    {
        NS_LOG_WARN("DataSnr reported to be zero; not saving this report.");
        return;
    }
    for (int i = 0; i < nSuccessfulMpdus; i++)
    {
        choosenMCS.emplace_back(station->m_lastMode.GetMcsValue());
    }
    station->m_lastSnrObserved = dataSnr;
    station->m_lastChannelWidthObserved = dataChannelWidth;
    station->m_lastNssObserved = dataNss;
}

void
AutoMcsWifiManager::DoReportFinalRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    Reset(station);
}

void
AutoMcsWifiManager::DoReportFinalDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    Reset(station);
}

WifiTxVector
AutoMcsWifiManager::DoGetDataTxVector(WifiRemoteStation* st, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << st << allowedWidth);
    AutoMcsWifiRemoteStation* station = static_cast<AutoMcsWifiRemoteStation*>(st);
    // We search within the Supported rate set the mode with the
    // highest data rate for which the SNR threshold is smaller than m_lastSnr
    // to ensure correct packet delivery.
    WifiMode maxMode = GetDefaultModeForSta(st);
    WifiTxVector txVector;
    WifiMode mode;
    uint64_t bestRate = 0;
    uint8_t selectedNss = 1;
    uint16_t guardInterval;
    uint16_t channelWidth = std::min(GetChannelWidth(station), allowedWidth);
    txVector.SetChannelWidth(channelWidth);

    if ((Simulator::Now().GetSeconds() < 10))
    {
        if ((station->m_lastSnrCached != CACHE_INITIAL_VALUE) &&
            (station->m_lastSnrObserved == station->m_lastSnrCached) &&
            (channelWidth == station->m_lastChannelWidth))
        {
            // SNR has not changed, so skip the search and use the last mode selected
            maxMode = station->m_lastMode;
            selectedNss = station->m_lastNss;
            NS_LOG_DEBUG("Using cached mode = " << maxMode.GetUniqueName() << " last snr observed "
                                                << station->m_lastSnrObserved << " cached "
                                                << station->m_lastSnrCached << " channel width "
                                                << station->m_lastChannelWidth << " nss "
                                                << +selectedNss);
        }

        else
        {
            if (GetHtSupported() && GetHtSupported(st))
            {
                for (uint8_t i = 0; i < GetNMcsSupported(station); i++)
                {
                    mode = GetMcsSupported(station, i);
                    txVector.SetMode(mode);
                    if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT)
                    {
                        guardInterval = static_cast<uint16_t>(
                            std::max(GetShortGuardIntervalSupported(station) ? 400 : 800,
                                     GetShortGuardIntervalSupported() ? 400 : 800));
                        txVector.SetGuardInterval(NanoSeconds(guardInterval));
                        // If the node and peer are both VHT capable, only search VHT modes
                        if (GetVhtSupported() && GetVhtSupported(station))
                        {
                            continue;
                        }
                        // If the node and peer are both HE capable, only search HE modes
                        if (GetHeSupported() && GetHeSupported(station))
                        {
                            continue;
                        }
                        // Derive NSS from the MCS index. There is a different mode for each
                        // possible NSS value.
                        uint8_t nss = (mode.GetMcsValue() / 8) + 1;
                        txVector.SetNss(nss);
                        if (!txVector.IsValid() || nss > std::min(GetMaxNumberOfTransmitStreams(),
                                                                  GetNumberOfSupportedStreams(st)))
                        {
                            NS_LOG_DEBUG("Skipping mode " << mode.GetUniqueName() << " nss " << +nss
                                                          << " width "
                                                          << txVector.GetChannelWidth());
                            continue;
                        }
                        double threshold = GetSnrThreshold(txVector);
                        uint64_t dataRate = mode.GetDataRate(txVector.GetChannelWidth(),
                                                             txVector.GetGuardInterval(),
                                                             nss);
                        NS_LOG_DEBUG("Testing mode " << mode.GetUniqueName() << " data rate "
                                                     << dataRate << " threshold " << threshold
                                                     << " last snr observed "
                                                     << station->m_lastSnrObserved << " cached "
                                                     << station->m_lastSnrCached);
                        double snr = GetLastObservedSnr(station, channelWidth, nss);
                        if (dataRate > bestRate && threshold < snr)
                        {
                            NS_LOG_DEBUG("Candidate mode = "
                                         << mode.GetUniqueName() << " data rate " << dataRate
                                         << " threshold " << threshold << " channel width "
                                         << channelWidth << " snr " << snr);
                            bestRate = dataRate;
                            maxMode = mode;
                            selectedNss = nss;
                        }
                    }
                    else if (mode.GetModulationClass() == WIFI_MOD_CLASS_VHT)
                    {
                        guardInterval = static_cast<uint16_t>(
                            std::max(GetShortGuardIntervalSupported(station) ? 400 : 800,
                                     GetShortGuardIntervalSupported() ? 400 : 800));
                        txVector.SetGuardInterval(NanoSeconds(guardInterval));
                        // If the node and peer are both HE capable, only search HE modes
                        if (GetHeSupported() && GetHeSupported(station))
                        {
                            continue;
                        }
                        // If the node and peer are not both VHT capable, only search HT modes
                        if (!GetVhtSupported() || !GetVhtSupported(station))
                        {
                            continue;
                        }
                        for (uint8_t nss = 1; nss <= std::min(GetMaxNumberOfTransmitStreams(),
                                                              GetNumberOfSupportedStreams(station));
                             nss++)
                        {
                            txVector.SetNss(nss);
                            if (!txVector.IsValid())
                            {
                                NS_LOG_DEBUG("Skipping mode " << mode.GetUniqueName() << " nss "
                                                              << +nss << " width "
                                                              << txVector.GetChannelWidth());
                                continue;
                            }
                            double threshold = GetSnrThreshold(txVector);
                            uint64_t dataRate = mode.GetDataRate(txVector.GetChannelWidth(),
                                                                 txVector.GetGuardInterval(),
                                                                 nss);
                            NS_LOG_DEBUG("Testing mode = "
                                         << mode.GetUniqueName() << " data rate " << dataRate
                                         << " threshold " << threshold << " last snr observed "
                                         << station->m_lastSnrObserved << " cached "
                                         << station->m_lastSnrCached);
                            double snr = GetLastObservedSnr(station, channelWidth, nss);
                            if (dataRate > bestRate && threshold < snr)
                            {
                                NS_LOG_DEBUG("Candidate mode = " << mode.GetUniqueName()
                                                                 << " data rate " << dataRate
                                                                 << " channel width "
                                                                 << channelWidth << " snr " << snr);
                                bestRate = dataRate;
                                maxMode = mode;
                                selectedNss = nss;
                            }
                        }
                    }
                    else // HE
                    {
                        guardInterval = std::max(GetGuardInterval(station).ToInteger(Time::NS), GetGuardInterval().ToInteger(Time::NS));
                        txVector.SetGuardInterval(NanoSeconds(guardInterval));
                        // If the node and peer are not both HE capable, only search (V)HT modes
                        if (!GetHeSupported() || !GetHeSupported(station))
                        {
                            continue;
                        }
                        for (uint8_t nss = 1; nss <= std::min(GetMaxNumberOfTransmitStreams(),
                                                              GetNumberOfSupportedStreams(station));
                             nss++)
                        {
                            txVector.SetNss(nss);
                            if (!txVector.IsValid())
                            {
                                NS_LOG_DEBUG("Skipping mode " << mode.GetUniqueName() << " nss "
                                                              << +nss << " width "
                                                              << +txVector.GetChannelWidth());
                                continue;
                            }
                            double threshold = GetSnrThreshold(txVector);
                            uint64_t dataRate = mode.GetDataRate(txVector.GetChannelWidth(),
                                                                 txVector.GetGuardInterval(),
                                                                 nss);
                            NS_LOG_DEBUG("Testing mode = "
                                         << mode.GetUniqueName() << " data rate " << dataRate
                                         << " threshold " << threshold << " last snr observed "
                                         << station->m_lastSnrObserved << " cached "
                                         << station->m_lastSnrCached);
                            double snr = GetLastObservedSnr(station, channelWidth, nss);
                            if (dataRate > bestRate && threshold < snr)
                            {
                                NS_LOG_DEBUG("Candidate mode = "
                                             << mode.GetUniqueName() << " data rate " << dataRate
                                             << " threshold " << threshold << " channel width "
                                             << channelWidth << " snr " << snr);
                                bestRate = dataRate;
                                maxMode = mode;
                                selectedNss = nss;
                            }
                        }
                    }
                }
            }
            else
            {
                // Non-HT selection
                selectedNss = 1;
                for (uint8_t i = 0; i < GetNSupported(station); i++)
                {
                    mode = GetSupported(station, i);
                    txVector.SetMode(mode);
                    txVector.SetNss(selectedNss);
                    uint16_t width = GetChannelWidthForNonHtMode(mode);
                    txVector.SetChannelWidth(width);
                    double threshold = GetSnrThreshold(txVector);
                    uint64_t dataRate = mode.GetDataRate(txVector.GetChannelWidth(),
                                                         txVector.GetGuardInterval(),
                                                         txVector.GetNss());
                    NS_LOG_DEBUG("mode = " << mode.GetUniqueName() << " threshold " << threshold
                                           << " last snr observed " << station->m_lastSnrObserved);
                    double snr = GetLastObservedSnr(station, width, 1);
                    if (dataRate > bestRate && threshold < snr)
                    {
                        NS_LOG_DEBUG("Candidate mode = " << mode.GetUniqueName() << " data rate "
                                                         << dataRate << " threshold " << threshold
                                                         << " snr " << snr);
                        bestRate = dataRate;
                        maxMode = mode;
                    }
                }
            }
            NS_LOG_DEBUG("Updating cached values for station to "
                         << maxMode.GetUniqueName() << " snr " << station->m_lastSnrObserved);
            station->m_lastSnrCached = station->m_lastSnrObserved;
            station->m_lastMode = maxMode;
            station->m_lastNss = selectedNss;
        }
    }
    else
    {
        double average = 0;
        for (auto it : choosenMCS)
        {
            average += it;
        }
        // std::cout << " raw value " << average / choosenMCS.size() << std::endl;
        average = std::ceil(average / choosenMCS.size());
        std::string mcs = "HeMcs" + std::to_string(int(average));
        maxMode = WifiMode(mcs);
    }
    NS_LOG_DEBUG("Found maxMode: " << maxMode << " channelWidth: " << channelWidth
                                   << " nss: " << +selectedNss);
    station->m_lastChannelWidth = channelWidth;
    if (maxMode.GetModulationClass() == WIFI_MOD_CLASS_HE)
    {
        guardInterval = std::max(GetGuardInterval(station).ToInteger(Time::NS), GetGuardInterval().ToInteger(Time::NS));
    }
    else if ((maxMode.GetModulationClass() == WIFI_MOD_CLASS_HT) ||
             (maxMode.GetModulationClass() == WIFI_MOD_CLASS_VHT))
    {
        guardInterval =
            static_cast<uint16_t>(std::max(GetShortGuardIntervalSupported(station) ? 400 : 800,
                                           GetShortGuardIntervalSupported() ? 400 : 800));
    }
    else
    {
        guardInterval = 800;
    }
    WifiTxVector bestTxVector{
        maxMode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(maxMode.GetModulationClass(), GetShortPreambleEnabled()),
        NanoSeconds(guardInterval),
        GetNumberOfAntennas(),
        selectedNss,
        0,
        GetPhy()->GetTxBandwidth(maxMode, channelWidth),
        GetAggregation(station)};

    uint64_t maxDataRate = maxMode.GetDataRate(bestTxVector);

    if (m_currentRate != maxDataRate)
    {
        // std::cout << "New datarate: " << maxMode << std::endl;

        m_currentRate = maxDataRate;
    }

    return bestTxVector;
}

WifiTxVector
AutoMcsWifiManager::DoGetRtsTxVector(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    if (!m_autoMCS)
    {
        AutoMcsWifiRemoteStation* station = static_cast<AutoMcsWifiRemoteStation*>(st);
        // We search within the Basic rate set the mode with the highest
        // SNR threshold possible which is smaller than m_lastSnr to
        // ensure correct packet delivery.
        double maxThreshold = 0.0;
        WifiTxVector txVector;
        WifiMode mode;
        uint8_t nss = 1;
        WifiMode maxMode = GetDefaultMode();
        // RTS is sent in a non-HT frame
        for (uint8_t i = 0; i < GetNBasicModes(); i++)
        {
            mode = GetBasicMode(i);
            txVector.SetMode(mode);
            txVector.SetNss(nss);
            txVector.SetChannelWidth(GetChannelWidthForNonHtMode(mode));
            double threshold = GetSnrThreshold(txVector);
            if (threshold > maxThreshold && threshold < station->m_lastSnrObserved)
            {
                maxThreshold = threshold;
                maxMode = mode;
            }
        }
        return WifiTxVector(
            maxMode,
            GetDefaultTxPowerLevel(),
            GetPreambleForTransmission(maxMode.GetModulationClass(), GetShortPreambleEnabled()),
            NanoSeconds(800),
            GetNumberOfAntennas(),
            nss,
            0,
            GetChannelWidthForNonHtMode(maxMode),
            GetAggregation(station));
    }
    else
    {
        return WifiTxVector(
            WifiMode("OfdmRate6Mbps"),
            GetDefaultTxPowerLevel(),
            GetPreambleForTransmission(WifiMode("OfdmRate6Mbps").GetModulationClass(),
                                       GetShortPreambleEnabled()),
            GetGuardInterval(st),
            1,
            1,
            0,
            GetPhy()->GetTxBandwidth(WifiMode("OfdmRate6Mbps"), GetChannelWidth(st)),
            GetAggregation(st));
    }
}

double
AutoMcsWifiManager::GetLastObservedSnr(AutoMcsWifiRemoteStation* station,
                                       uint16_t channelWidth,
                                       uint8_t nss) const
{
    double snr = station->m_lastSnrObserved;
    if (channelWidth != station->m_lastChannelWidthObserved)
    {
        snr /= (static_cast<double>(channelWidth) / station->m_lastChannelWidthObserved);
    }
    if (nss != station->m_lastNssObserved)
    {
        snr /= (static_cast<double>(nss) / station->m_lastNssObserved);
    }
    NS_LOG_DEBUG("Last observed SNR is " << station->m_lastSnrObserved << " for channel width "
                                         << station->m_lastChannelWidthObserved << " and nss "
                                         << +station->m_lastNssObserved << "; computed SNR is "
                                         << snr << " for channel width " << channelWidth
                                         << " and nss " << +nss);
    return snr;
}

} // namespace ns3
