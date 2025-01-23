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

#ifndef AI_CONSTANT_RATE_WIFI_MANAGER_H
#define AI_CONSTANT_RATE_WIFI_MANAGER_H

#include <ns3/ai-module.h>
#include <ns3/wifi-remote-station-manager.h>

namespace ns3
{

struct AiConstantRateEnvStruct
{
    uint8_t transmitStreams;
    uint8_t supportedStreams;
    uint8_t mcs;

    AiConstantRateEnvStruct()
        : transmitStreams(0),
          supportedStreams(0),
          mcs(0)
    {
    }
};

struct AiConstantRateActStruct
{
    uint8_t nss;
    uint8_t next_mcs;

    AiConstantRateActStruct()
        : nss(0),
          next_mcs(0)
    {
    }
};

/**
 * \ingroup wifi
 * \brief use constant rates for data and RTS transmissions
 *
 * This class uses always the same transmission rate for every
 * packet sent.
 */
class AiConstantRateWifiManager : public WifiRemoteStationManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    AiConstantRateWifiManager();
    ~AiConstantRateWifiManager() override;

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
    void DoReportFinalRtsFailed(WifiRemoteStation* station) override;
    void DoReportFinalDataFailed(WifiRemoteStation* station) override;
    WifiTxVector DoGetDataTxVector(WifiRemoteStation* station, MHz_u allowedWidth) override;
    WifiTxVector DoGetRtsTxVector(WifiRemoteStation* station) override;

    WifiMode m_dataMode; //!< Wifi mode for unicast Data frames
    WifiMode m_ctlMode;  //!< Wifi mode for RTS frames
};

} // namespace ns3

#endif /* AI_CONSTANT_RATE_WIFI_MANAGER_H */
