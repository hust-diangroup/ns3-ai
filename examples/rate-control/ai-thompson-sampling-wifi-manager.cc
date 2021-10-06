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
 */

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/core-module.h"
#include "ns3/packet.h"

#include "ns3/wifi-phy.h"

#include "ai-thompson-sampling-wifi-manager.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace ns3 {

/**
 * A structure containing parameters of a single rate and its
 * statistics.
 */
struct AiRateStats {
  WifiMode mode; ///< MCS
  uint16_t channelWidth; ///< channel width in MHz
  uint8_t nss; ///< Number of spatial streams
};

GlobalValue gNS3AIRLUID (
  "NS3AIRLUID", 
  "UID of Ns3AIRL",
  UintegerValue (2333),
  MakeUintegerChecker<uint16_t> ()
);

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

NS_OBJECT_ENSURE_REGISTERED (AiThompsonSamplingWifiManager);

NS_LOG_COMPONENT_DEFINE ("AiThompsonSamplingWifiManager");

TypeId
AiThompsonSamplingWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AiThompsonSamplingWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<AiThompsonSamplingWifiManager> ()
    .AddAttribute ("Decay",
                   "Exponential decay coefficient, Hz; zero is a valid value for static scenarios",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&AiThompsonSamplingWifiManager::m_decay),
                   MakeDoubleChecker<double> (0.0))
    .AddTraceSource ("Rate",
                     "Traced value for rate changes (b/s)",
                     MakeTraceSourceAccessor (&AiThompsonSamplingWifiManager::m_currentRate),
                     "ns3::TracedValueCallback::Uint64")
  ;
  return tid;
}

AiThompsonSamplingWifiManager::AiThompsonSamplingWifiManager ()
  : m_currentRate{0}
{
  NS_LOG_FUNCTION (this);
  
  UintegerValue uv;
  gNS3AIRLUID.GetValue (uv);
  m_ns3ai_id = uv.Get ();
  NS_LOG_UNCOND("m_ns3ai_id" << m_ns3ai_id);

  m_ns3ai_mod = new Ns3AIRL<AiThompsonSamplingEnv, AiThompsonSamplingAct> (m_ns3ai_id);
  m_ns3ai_mod->SetCond (2, 0);

  // set input, type 0x01
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x01;
  env->data.addr = static_cast<void *> (this);
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  m_ns3ai_manager_id = act->managerId;
  m_ns3ai_mod->GetCompleted ();
}

AiThompsonSamplingWifiManager::~AiThompsonSamplingWifiManager ()
{
  NS_LOG_FUNCTION (this);
  delete m_ns3ai_mod;
}

WifiRemoteStation *
AiThompsonSamplingWifiManager::DoCreateStation () const
{
  NS_LOG_FUNCTION (this);
  AiThompsonSamplingWifiRemoteStation *station = new AiThompsonSamplingWifiRemoteStation ();

  // set input, type 0x02
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x02;
  env->managerId = m_ns3ai_manager_id;
  env->data.addr = static_cast<void *> (station);
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  station->m_ns3ai_station_id = act->stationId;
  m_ns3ai_mod->GetCompleted ();

  return station;
}

void
AiThompsonSamplingWifiManager::InitializeStation (WifiRemoteStation *st) const
{
  auto station = static_cast<AiThompsonSamplingWifiRemoteStation *> (st);
  if (!station->m_mcsStats.empty ())
    {
      return;
    }

  // Add HT, VHT or HE MCSes
  for (const auto &mode : GetPhy ()->GetMcsList ())
    {
      for (uint16_t j = 20; j <= GetPhy ()->GetChannelWidth (); j *= 2)
        {
          WifiModulationClass modulationClass = WIFI_MOD_CLASS_HT;
          if (GetVhtSupported ())
            {
              modulationClass = WIFI_MOD_CLASS_VHT;
            }
          if (GetHeSupported ())
            {
              modulationClass = WIFI_MOD_CLASS_HE;
            }
          if (mode.GetModulationClass () == modulationClass)
            {
              for (uint8_t k = 1; k <= GetPhy ()->GetMaxSupportedTxSpatialStreams (); k++)
                {
                  if (mode.IsAllowed (j, k))
                    {
                      AiRateStats stats;
                      stats.mode = mode;
                      stats.channelWidth = j;
                      stats.nss = k;

                      station->m_mcsStats.push_back (stats);
                    }
                }
            }
        }
    }

  if (station->m_mcsStats.empty ())
    {
      // Add legacy non-HT modes.
      for (uint8_t i = 0; i < GetNSupported (station); i++)
        {
          AiRateStats stats;
          stats.mode = GetSupported (station, i);
          if (stats.mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS
              || stats.mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
            {
              stats.channelWidth = 22;
            }
          else
            {
              stats.channelWidth = 20;
            }
          stats.nss = 1;
          station->m_mcsStats.push_back (stats);
        }
    }

  NS_ASSERT_MSG (!station->m_mcsStats.empty (), "No usable MCS found");

  // set input, type 0x03
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x03;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;

  NS_ASSERT_MSG (station->m_mcsStats.size () <= 64, "m_mcsStats too long");
  memset (env->data.stats, 0, sizeof (env->data.stats));
  auto s = env->data.stats;
  for (size_t i = 0; i < station->m_mcsStats.size (); i++)
    {
      const WifiMode mode{station->m_mcsStats.at (i).mode};
      s[i].nss = station->m_mcsStats[i].nss;
      s[i].channelWidth = station->m_mcsStats[i].channelWidth;
      s[i].guardInterval = GetModeGuardInterval (st, mode);
      s[i].dataRate = mode.GetDataRate (s[i].channelWidth, s[i].guardInterval, s[i].nss);
    }
  s[station->m_mcsStats.size ()].lastDecay = -1.0; // mark end of array
  std::cout << (int) env->stationId << ' ' << station << ' ' <<
    station->m_mcsStats.size () << std::endl;
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  NS_ASSERT_MSG (act->stationId == env->stationId, "Error 0x03");
  m_ns3ai_mod->GetCompleted ();

  UpdateNextMode (st);
}

void
AiThompsonSamplingWifiManager::DoReportRxOk (WifiRemoteStation *station, double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}

void
AiThompsonSamplingWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
AiThompsonSamplingWifiManager::DoReportDataFailed (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  InitializeStation (st);
  auto station = static_cast<AiThompsonSamplingWifiRemoteStation *> (st);

  // set input, type 0x05
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x05;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;
  env->data.decay.decay = m_decay;
  env->data.decay.now = Simulator::Now ().GetSeconds ();
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  NS_ASSERT_MSG (act->stationId == env->stationId, "Error 0x05");
  m_ns3ai_mod->GetCompleted ();
}

void
AiThompsonSamplingWifiManager::DoReportRtsOk (WifiRemoteStation *st, double ctsSnr, WifiMode ctsMode,
                                     double rtsSnr)
{
  NS_LOG_FUNCTION (this << st << ctsSnr << ctsMode.GetUniqueName () << rtsSnr);
}

void
AiThompsonSamplingWifiManager::UpdateNextMode (WifiRemoteStation *st) const
{
  InitializeStation (st);
  auto station = static_cast<AiThompsonSamplingWifiRemoteStation *> (st);
  NS_ASSERT (!station->m_mcsStats.empty ());

  // set input, type 0x0a
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x0a;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;
  env->data.decay.decay = m_decay;
  env->data.decay.now = Simulator::Now ().GetSeconds ();
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  NS_ASSERT_MSG (act->stationId == env->stationId, "Error 0x0a");
  m_ns3ai_mod->GetCompleted ();
}

void
AiThompsonSamplingWifiManager::DoReportDataOk (WifiRemoteStation *st, double ackSnr, WifiMode ackMode,
                                      double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode.GetUniqueName () << dataSnr);
  InitializeStation (st);
  auto station = static_cast<AiThompsonSamplingWifiRemoteStation *> (st);

  // set input, type 0x06
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x06;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;
  env->data.decay.decay = m_decay;
  env->data.decay.now = Simulator::Now ().GetSeconds ();
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  NS_ASSERT_MSG (act->stationId == env->stationId, "Error 0x06");
  m_ns3ai_mod->GetCompleted ();
}

void
AiThompsonSamplingWifiManager::DoReportAmpduTxStatus (WifiRemoteStation *st, uint16_t nSuccessfulMpdus,
                                             uint16_t nFailedMpdus, double rxSnr, double dataSnr,
                                             uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << nSuccessfulMpdus << nFailedMpdus << rxSnr << dataSnr);
  InitializeStation (st);
  auto station = static_cast<AiThompsonSamplingWifiRemoteStation *> (st);

  // set input, type 0x07
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x07;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;
  env->var = (uint64_t) nSuccessfulMpdus << 32 | nFailedMpdus;
  env->data.decay.decay = m_decay;
  env->data.decay.now = Simulator::Now ().GetSeconds ();
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  NS_ASSERT_MSG (act->stationId == env->stationId, "Error 0x07");
  m_ns3ai_mod->GetCompleted ();
}

void
AiThompsonSamplingWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
AiThompsonSamplingWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

uint16_t
AiThompsonSamplingWifiManager::GetModeGuardInterval (WifiRemoteStation *st, WifiMode mode) const
{
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      return std::max (GetGuardInterval (st), GetGuardInterval ());
    }
  else if ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT) ||
           (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
    {
      return std::max<uint16_t> (GetShortGuardIntervalSupported (st) ? 400 : 800,
                                 GetShortGuardIntervalSupported () ? 400 : 800);
    }
  else
    {
      return 800;
    }
}

WifiTxVector
AiThompsonSamplingWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  InitializeStation (st);
  auto station = static_cast<AiThompsonSamplingWifiRemoteStation *> (st);

  // set input, type 0x08
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x08;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  WifiMode mode = station->m_mcsStats.at (act->res).mode;
  uint8_t nss = act->stats.nss;
  uint16_t channelWidth = std::min (act->stats.channelWidth, GetPhy ()->GetChannelWidth ());
  uint16_t guardInterval = act->stats.guardInterval;
  m_ns3ai_mod->GetCompleted ();

  uint64_t rate = mode.GetDataRate (channelWidth, guardInterval, nss);
  if (m_currentRate != rate)
    {
      NS_LOG_DEBUG ("New datarate: " << rate);
      m_currentRate = rate;
    }

  return WifiTxVector (
      mode,
      GetDefaultTxPowerLevel (),
      GetPreambleForTransmission (mode.GetModulationClass (),
                                  GetShortPreambleEnabled ()),
      guardInterval,
      GetNumberOfAntennas (),
      nss,
      0, // NESS
      GetChannelWidthForTransmission (mode, channelWidth),
      GetAggregation (station),
      false);
}

WifiTxVector
AiThompsonSamplingWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  InitializeStation (st);
  auto station = static_cast<AiThompsonSamplingWifiRemoteStation *> (st);

  // set input, type 0x09
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x09;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  WifiMode mode = station->m_mcsStats.at (act->res).mode;
  uint8_t nss = act->stats.nss;
  uint16_t channelWidth = std::min (act->stats.channelWidth, GetPhy ()->GetChannelWidth ());
  uint16_t guardInterval = act->stats.guardInterval;
  m_ns3ai_mod->GetCompleted ();

  // Make sure control frames are sent using 1 spatial stream.
  NS_ASSERT (nss == 1);

  return WifiTxVector (
      mode, GetDefaultTxPowerLevel (),
      GetPreambleForTransmission (mode.GetModulationClass (), GetShortPreambleEnabled ()),
      guardInterval,
      GetNumberOfAntennas (),
      nss,
      0, // NESS
      GetChannelWidthForTransmission (mode, channelWidth),
      GetAggregation (station),
      false);
}

} //namespace ns3
