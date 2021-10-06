/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Based on 'src/wifi/model/rate-control/constant-rate-wifi-manager.cc'
 */

#include "ns3/string.h"
#include "ns3/log.h"
#include "ai-constant-rate-wifi-manager.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/wifi-utils.h"

#define Min(a,b) ((a < b) ? a : b)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AiConstantRateWifiManager");

NS_OBJECT_ENSURE_REGISTERED (AiConstantRateWifiManager);

TypeId
AiConstantRateWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AiConstantRateWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<AiConstantRateWifiManager> ()
    .AddAttribute ("DataMode", "The transmission mode to use for every data packet transmission",
                   StringValue ("OfdmRate6Mbps"),
                   MakeWifiModeAccessor (&AiConstantRateWifiManager::m_dataMode),
                   MakeWifiModeChecker ())
    .AddAttribute ("ControlMode", "The transmission mode to use for every RTS packet transmission.",
                   StringValue ("OfdmRate6Mbps"),
                   MakeWifiModeAccessor (&AiConstantRateWifiManager::m_ctlMode),
                   MakeWifiModeChecker ())
  ;
  return tid;
}

AiConstantRateWifiManager::AiConstantRateWifiManager (uint16_t id = 2333) : m_ns3ai_id (id)
{
  m_ns3ai_mod = new Ns3AIRL<AiConstantRateEnv, AiConstantRateAct> (id);
  m_ns3ai_mod->SetCond (2, 0);
  NS_LOG_FUNCTION (this);
}

AiConstantRateWifiManager::~AiConstantRateWifiManager ()
{
  delete m_ns3ai_mod;
  NS_LOG_FUNCTION (this);
}

WifiRemoteStation *
AiConstantRateWifiManager::DoCreateStation (void) const
{
  NS_LOG_FUNCTION (this);
  WifiRemoteStation *station = new WifiRemoteStation ();
  return station;
}

void
AiConstantRateWifiManager::DoReportRxOk (WifiRemoteStation *station,
                                       double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}

void
AiConstantRateWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
AiConstantRateWifiManager::DoReportDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
AiConstantRateWifiManager::DoReportRtsOk (WifiRemoteStation *st,
                                        double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << st << ctsSnr << ctsMode << rtsSnr);
}

void
AiConstantRateWifiManager::DoReportDataOk (WifiRemoteStation *st, double ackSnr, WifiMode ackMode,
                                         double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode << dataSnr << dataChannelWidth << +dataNss);
}

void
AiConstantRateWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
AiConstantRateWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

WifiTxVector
AiConstantRateWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);

  // set input
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->transmitStreams = GetMaxNumberOfTransmitStreams ();
  env->supportedStreams = GetNumberOfSupportedStreams (st);
  if (m_dataMode.GetModulationClass () == WIFI_MOD_CLASS_HT)
    env->mcs = m_dataMode.GetMcsValue ();
  else
    env->mcs = 0xffu;
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  uint8_t nss = act->nss;
  uint8_t next_mcs = act->next_mcs;
  NS_LOG_FUNCTION (next_mcs);
  m_ns3ai_mod->GetCompleted ();
  
  // uncomment to specify arbitrary MCS
  // m_dataMode = GetMcsSupported (st, next_mcs);

  return WifiTxVector (m_dataMode, GetDefaultTxPowerLevel (), GetPreambleForTransmission (m_dataMode.GetModulationClass (), GetShortPreambleEnabled ()), ConvertGuardIntervalToNanoSeconds (m_dataMode, GetShortGuardIntervalSupported (st), NanoSeconds (GetGuardInterval (st))), GetNumberOfAntennas (), nss, 0, GetChannelWidthForTransmission (m_dataMode, GetChannelWidth (st)), GetAggregation (st));
}

WifiTxVector
AiConstantRateWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  return WifiTxVector (m_ctlMode, GetDefaultTxPowerLevel (), GetPreambleForTransmission (m_ctlMode.GetModulationClass (), GetShortPreambleEnabled ()), ConvertGuardIntervalToNanoSeconds (m_ctlMode, GetShortGuardIntervalSupported (st), NanoSeconds (GetGuardInterval (st))), 1, 1, 0, GetChannelWidthForTransmission (m_ctlMode, GetChannelWidth (st)), GetAggregation (st));
}

} //namespace ns3
