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

#ifndef AI_THOMPSON_SAMPLING_WIFI_MANAGER_H
#define AI_THOMPSON_SAMPLING_WIFI_MANAGER_H

#include <array>

#include <ns3/ns3-ai-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/wifi-remote-station-manager.h>
#include <ns3/traced-value.h>

namespace ns3
{

typedef struct ThompsonSamplingRateStats
{
    uint8_t nss;
    uint16_t channelWidth;
    uint16_t guardInterval;
    uint64_t dataRate;
    double success;
    double fails;
    double lastDecay; // Time
} ThompsonSamplingRateStats;

typedef struct ThompsonSamplingEnvDecay
{
    int8_t decayIdx;
    double decay;
    double now; // Time
} ThompsonSamplingEnvDecay;

//typedef union ThompsonSamplingEnvPayloadUnion {
//    std::array<ThompsonSamplingRateStats, 64> stats;
//    ThompsonSamplingEnvDecay decay;
//} ThompsonSamplingEnvPayloadUnion;

typedef struct ThompsonSamplingEnvPayloadStruct {
    std::array<ThompsonSamplingRateStats, 64> stats;
    ThompsonSamplingEnvDecay decay;
} ThompsonSamplingEnvPayloadStruct;

typedef struct AiThompsonSamplingEnvStruct
{
    int8_t type;
    int8_t managerId;
    int8_t stationId;
    uint64_t var;
    ThompsonSamplingEnvPayloadStruct data;
} AiThompsonSamplingEnvStruct;

typedef struct AiThompsonSamplingActStruct
{
    int8_t managerId;
    int8_t stationId;
    uint64_t res;
    ThompsonSamplingRateStats stats;
} AiThompsonSamplingActStruct;

/**
 * \brief Thompson Sampling rate control algorithm
 * \ingroup wifi
 *
 * This class implements Thompson Sampling rate control algorithm.
 *
 * It was implemented for use as a baseline in
 * https://doi.org/10.1109/ACCESS.2020.3023552
 */
class AiThompsonSamplingWifiManager : public WifiRemoteStationManager
{
  public:
    static TypeId GetTypeId();
    AiThompsonSamplingWifiManager();
    ~AiThompsonSamplingWifiManager() override;

  private:
    WifiRemoteStation* DoCreateStation() const override;
    void DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode) override;
    void DoReportRtsFailed(WifiRemoteStation* station) override;
    void DoReportDataFailed(WifiRemoteStation* station) override;
    void DoReportRtsOk(WifiRemoteStation* station,
                       double ctsSnr,
                       WifiMode ctsMode,
                       double rtsSnr) override;
    void DoReportDataOk(WifiRemoteStation* station,
                        double ackSnr,
                        WifiMode ackMode,
                        double dataSnr,
                        uint16_t dataChannelWidth,
                        uint8_t dataNss) override;
    void DoReportAmpduTxStatus(WifiRemoteStation* station,
                               uint16_t nSuccessfulMpdus,
                               uint16_t nFailedMpdus,
                               double rxSnr,
                               double dataSnr,
                               uint16_t dataChannelWidth,
                               uint8_t dataNss) override;
    void DoReportFinalRtsFailed(WifiRemoteStation* station) override;
    void DoReportFinalDataFailed(WifiRemoteStation* station) override;
    WifiTxVector DoGetDataTxVector(WifiRemoteStation* station, uint16_t allowedWidth) override;
    WifiTxVector DoGetRtsTxVector(WifiRemoteStation* station) override;

    /**
     * Initializes station rate tables. If station is already initialized,
     * nothing is done.
     *
     * \param station Station which should be initialized.
     */
    void InitializeStation(WifiRemoteStation* station) const;

    /**
     * Draws a new MCS and related parameters to try next time for this
     * station.
     *
     * This method should only be called between TXOPs to avoid sending
     * multiple frames using different modes. Otherwise it is impossible
     * to tell which mode was used for succeeded/failed frame when
     * feedback is received.
     *
     * \param station Station for which a new mode should be drawn.
     */
    void UpdateNextMode(WifiRemoteStation* station) const;

    /**
     * Returns guard interval in nanoseconds for the given mode.
     *
     * \param st Remote STA.
     * \param mode The WifiMode.
     * \return the guard interval in nanoseconds
     */
    uint16_t GetModeGuardInterval(WifiRemoteStation* st, WifiMode mode) const;

    double m_decay; //!< Exponential decay coefficient, Hz

    TracedValue<uint64_t> m_currentRate; //!< Trace rate changes

    //  uint16_t m_ns3ai_id;
//    NS3AIRL<AiThompsonSamplingEnv, AiThompsonSamplingAct> *m_ns3ai_mod;
    int8_t m_ns3ai_manager_id;
    std::vector<AiThompsonSamplingEnvStruct> *m_temp_env;
    std::vector<AiThompsonSamplingActStruct> *m_temp_act;
};

} // namespace ns3

#endif /* AI_THOMPSON_SAMPLING_WIFI_MANAGER_H */
