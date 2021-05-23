#include "cqi-dl-env.h"

/**
 * \brief Link the shared memory with the id and set the operation lock
 * 
 * \param[in] id  shared memory id, should be the same in python and ns-3
 */
namespace ns3 {
CQIDL::CQIDL (uint16_t id) : Ns3AIDL<CqiFeature, CqiPredicted, CQITarget> (id)
{
  SetCond (2, 0);
}

/**
 * \brief Set the value of wbcqi.
 * 
 * \param[in] cqi  the value of wbcqi to be set
 */
void
CQIDL::SetWbCQI (uint8_t cqi)
{
  auto feature = FeatureSetterCond ();    ///< get pointer to modify feature
  feature->wbCqi = cqi;
  SetCompleted ();                        ///< modification completed
}

/**
 * \brief Get the predictive value of wbcqi.
 * 
 * \returns the predictive value of wbcqi
 */
uint8_t
CQIDL::GetWbCQI (void)
{
  auto pred = PredictedGetterCond ();     ///< get predicted pointer for reading
  uint8_t ret = pred->new_wbCqi;
  GetCompleted ();                        ///< read completed
  return ret;
}

/**
 * \brief Set the value of sbCqi, rbgNum and nLayers.
 * 
 * \param[in] cqi  the means result of sbcqi
 * 
 * \param[in] nLayers  the value of nLayers to be set
 */
void
CQIDL::SetSbCQI (SbMeasResult_s cqi, uint32_t nLayers)
{
  auto feature = FeatureSetterCond ();    ///< get pointer to modify feature
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
  SetCompleted ();                        ///< modification completed
}

/**
 * \brief Get the predictive value of sbcqi.
 * 
 * \param[out] cqi  the address of cqi
 */
void
CQIDL::GetSbCQI (SbMeasResult_s &cqi)
{
  auto feature = FeatureGetterCond ();    ///< get feature pointer for reading
  auto pred = PredictedGetterCond ();     ///< get predicted pointer for reading
  uint32_t rbgNum = feature->rbgNum;
  uint32_t nLayers = feature->nLayers;

  for (uint32_t i = 0; i < rbgNum; ++i)
    {
      for (uint32_t j = 0; j < nLayers; ++j)
        {
          cqi.m_higherLayerSelected.at (i).m_sbCqi.at (j) = pred->new_sbCqi[i][j];
        }
    }
  GetCompleted ();                        ///< read completed
}

/**
 * \brief Set the target.
 * 
 * \param[in] tar  the value of target to be set
 */
void CQIDL::SetTarget (uint8_t tar)
{
  auto target = TargetSetterCond ();      ///< get pointer to modify target
  target->target = tar;
  SetCompleted ();                        ///< modification completed
}

/**
 * \brief Get the target.
 * 
 * \returns the value of target
 */
uint8_t CQIDL::GetTarget (void)
{
  auto tar = TargetGetterCond ();         ///< get target pointer for reading
  uint8_t ret = tar->target;
  GetCompleted ();                        ///< read completed
  return ret;
}
} // namespace ns3
