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

#ifndef AI_THOMPSON_SAMPLING_WIFI_MANAGER_H
#define AI_THOMPSON_SAMPLING_WIFI_MANAGER_H

#include <ns3/ai-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/traced-value.h>
#include <ns3/wifi-remote-station-manager.h>

#include <array>

namespace ns3
{

struct ThompsonSamplingRateStats
{
    uint8_t nss;
    uint16_t channelWidth;
    uint16_t guardInterval;
    uint64_t dataRate;
    double success;
    double fails;
    double lastDecay;

    ThompsonSamplingRateStats()
        : nss(0),
          channelWidth(0),
          guardInterval(0),
          dataRate(0),
          success(0),
          fails(0),
          lastDecay(0)
    {
    }
};

struct ThompsonSamplingEnvDecay
{
    int8_t decayIdx;
    double decay;
    double now;

    ThompsonSamplingEnvDecay()
        : decayIdx(0),
          decay(0),
          now(0)
    {
    }
};

struct ThompsonSamplingEnvPayloadStruct
{
    std::array<ThompsonSamplingRateStats, 64> stats;
    ThompsonSamplingEnvDecay decay;

    ThompsonSamplingEnvPayloadStruct()
        : stats(),
          decay()
    {
    }
};

struct AiThompsonSamplingEnvStruct
{
    int8_t type;
    int8_t managerId;
    int8_t stationId;
    uint64_t var;
    ThompsonSamplingEnvPayloadStruct data;

    AiThompsonSamplingEnvStruct()
        : type(0),
          managerId(0),
          stationId(0),
          var(0),
          data()
    {
    }
};

struct AiThompsonSamplingActStruct
{
    int8_t managerId;
    int8_t stationId;
    uint64_t res;
    ThompsonSamplingRateStats stats;

    AiThompsonSamplingActStruct()
        : managerId(0),
          stationId(0),
          res(0),
          stats()
    {
    }
};

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
                        MHz_u dataChannelWidth,
                        uint8_t dataNss) override;
    void DoReportAmpduTxStatus(WifiRemoteStation* station,
                               uint16_t nSuccessfulMpdus,
                               uint16_t nFailedMpdus,
                               double rxSnr,
                               double dataSnr,
                               MHz_u dataChannelWidth,
                               uint8_t dataNss) override;
    void DoReportFinalRtsFailed(WifiRemoteStation* station) override;
    void DoReportFinalDataFailed(WifiRemoteStation* station) override;
    WifiTxVector DoGetDataTxVector(WifiRemoteStation* station, MHz_u allowedWidth) override;
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

    int8_t m_ns3ai_manager_id;
};

} // namespace ns3

#endif /* AI_THOMPSON_SAMPLING_WIFI_MANAGER_H */
