#pragma once
#include "ns3/ns3-ai-dl.h"
#include "ns3/ff-mac-common.h"

namespace ns3 {
#define MAX_RBG_NUM 32
struct CqiFeature
{
  uint8_t wbCqi;
  uint8_t rbgNum;
  uint8_t nLayers;
  uint8_t sbCqi[MAX_RBG_NUM][2];
};
struct CqiPredicted
{
  uint8_t new_wbCqi;
  uint8_t new_sbCqi[MAX_RBG_NUM][2];
};
struct CQITarget
{
  uint8_t target;
};
class CQIDL : public Ns3AIDL<CqiFeature, CqiPredicted, CQITarget>
{
public:
  CQIDL (void) = delete;
  CQIDL (uint16_t id);
  void SetWbCQI (uint8_t cqi);
  uint8_t GetWbCQI (void);
  void SetSbCQI (SbMeasResult_s cqi, uint32_t nLayers);
  void GetSbCQI (SbMeasResult_s &cqi);
  void SetTarget (uint8_t tar);
  uint8_t GetTarget (void);
};

} // namespace ns3
