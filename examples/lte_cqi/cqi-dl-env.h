#pragma once
#include "ns3/ns3-ai-dl.h"
#include "ns3/ff-mac-common.h"

namespace ns3 {
#define MAX_RBG_NUM 32

/**
 * \brief The feature of cqi.
 * 
 * The feature of DL training (in this example, feature of cqi)
 * shared between ns-3 and python with the same shared memory
 * using the ns3-ai model.
 */
struct CqiFeature
{
  uint8_t wbCqi;                  ///< wide band cqi
  uint8_t rbgNum;                 ///< resource block group number
  uint8_t nLayers;                ///< number of layers
  uint8_t sbCqi[MAX_RBG_NUM][2];  ///< sub band cqi
};

/**
 * \brief The prediction of cqi.
 * 
 * The prediction of DL training (in this example, prediction of cqi)
 * calculated by python and put back to ns-3 with the shared memory.
 */
struct CqiPredicted
{
  uint8_t new_wbCqi;
  uint8_t new_sbCqi[MAX_RBG_NUM][2];
};

/**
 * \brief The target of cqi.
 * 
 * The target of DL training (in this example, target of cqi)
 */
struct CQITarget
{
  uint8_t target;
};

/**
 * \brief A class to predict CQI(Channel Quality Indication).
 *
 * This class shared memory with python by the same id.
 * It set data through member function 'Set[xxx]()', 
 * and put them into the shared memory, using python to calculate,
 * and got prediction through member function 'Get[xxx]()'.
 */
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
