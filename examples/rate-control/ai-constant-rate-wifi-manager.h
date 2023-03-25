/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 */


#ifndef AI_CONSTANT_RATE_WIFI_MANAGER_H
#define AI_CONSTANT_RATE_WIFI_MANAGER_H

#include "ns3/wifi-remote-station-manager.h"
#include "ns3/ns3-ai-module.h"

namespace ns3 {

struct AiConstantRateEnv
{
  uint8_t transmitStreams;
  uint8_t supportedStreams;
  uint8_t mcs;
} Packed;

struct AiConstantRateAct
{
  uint8_t nss;
  uint8_t next_mcs;
} Packed;

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
  static TypeId GetTypeId (void);
  AiConstantRateWifiManager (uint16_t id = 2333);
  virtual ~AiConstantRateWifiManager ();


private:
  WifiRemoteStation* DoCreateStation (void) const override;
  void DoReportRxOk (WifiRemoteStation *station,
                     double rxSnr, WifiMode txMode) override;
  void DoReportRtsFailed (WifiRemoteStation *station) override;
  void DoReportDataFailed (WifiRemoteStation *station) override;
  void DoReportRtsOk (WifiRemoteStation *station,
                      double ctsSnr, WifiMode ctsMode, double rtsSnr) override;
  void DoReportDataOk (WifiRemoteStation *station, double ackSnr, WifiMode ackMode,
                       double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss) override;
  void DoReportFinalRtsFailed (WifiRemoteStation *station) override;
  void DoReportFinalDataFailed (WifiRemoteStation *station) override;
  WifiTxVector DoGetDataTxVector (WifiRemoteStation *station, uint16_t allowedWidth) override;
  WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station) override;

  WifiMode m_dataMode; //!< Wifi mode for unicast Data frames
  WifiMode m_ctlMode;  //!< Wifi mode for RTS frames

//  uint16_t m_ns3ai_id;
  Ns3AIRL<AiConstantRateEnv, AiConstantRateAct> * m_ns3ai_mod;
};

} //namespace ns3

#endif /* AI_CONSTANT_RATE_WIFI_MANAGER_H */
