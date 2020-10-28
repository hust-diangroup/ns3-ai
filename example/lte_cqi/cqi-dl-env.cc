#include "cqi-dl-env.h"

namespace ns3 {
CQIDL::CQIDL (uint16_t id) : Ns3AIDL<CqiFeature, CqiPredicted, CQITarget> (id)
{
  SetCond (2, 0);
}

void
CQIDL::SetWbCQI (uint8_t cqi)
{
  auto feature = FeatureSetterCond ();
  feature->wbCqi = cqi;
  SetCompleted ();
}
uint8_t
CQIDL::GetWbCQI (void)
{
  auto pred = PredictedGetterCond ();
  uint8_t ret = pred->new_wbCqi;
  GetCompleted ();
  return ret;
}

void
CQIDL::SetSbCQI (SbMeasResult_s cqi, uint32_t nLayers)
{
  auto feature = FeatureSetterCond ();
  uint32_t rbgNum = cqi.m_higherLayerSelected.size ();
  feature->rbgNum = rbgNum;
  feature->nLayers = nLayers;
  for (uint32_t i = 0; i < rbgNum; ++i)
    {
      for (uint32_t j = 0; j < nLayers; ++j)
        {
          feature->sbCqi[i][j] = cqi.m_higherLayerSelected.at (i).m_sbCqi.at (j);
        }
    }
  SetCompleted ();
}
void
CQIDL::GetSbCQI (SbMeasResult_s &cqi)
{
  auto feature = FeatureGetterCond ();
  auto pred = PredictedGetterCond ();
  uint32_t rbgNum = feature->rbgNum;
  uint32_t nLayers = feature->nLayers;

  for (uint32_t i = 0; i < rbgNum; ++i)
    {
      for (uint32_t j = 0; j < nLayers; ++j)
        {
          cqi.m_higherLayerSelected.at (i).m_sbCqi.at (j) = pred->new_sbCqi[i][j];
        }
    }
  GetCompleted ();
}

void CQIDL::SetTarget (uint8_t tar)
{
  auto target = TargetSetterCond ();
  target->target = tar;
  SetCompleted ();
}
uint8_t CQIDL::GetTarget (void)
{
  auto tar = TargetGetterCond ();
  uint8_t ret = tar->target;
  GetCompleted ();
  return ret;
}
} // namespace ns3
