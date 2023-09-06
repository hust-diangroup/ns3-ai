/*
 * Copyright (c) 2019-2023 Huazhong University of Science and Technology
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
 * Authors: Pengyu Liu <eic_lpy@hust.edu.cn>
 *          Hao Yin <haoyin@uw.edu>
 *          Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#pragma once
#include "ns3/ai-module.h"
#include "ns3/core-module.h"
#include "ns3/ff-mac-common.h"

namespace ns3
{
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
    uint8_t wbCqi; ///< wide band cqi
    //  uint8_t rbgNum;                 ///< resource block group number
    //  uint8_t nLayers;                ///< number of layers
    //  uint8_t sbCqi[MAX_RBG_NUM][2];  ///< sub band cqi
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
    //  uint8_t new_sbCqi[MAX_RBG_NUM][2];
};

/**
 * \brief A class to predict CQI(Channel Quality Indication).
 *
 * This class shared memory with python by the same id.
 * It set data through member function 'Set[xxx]()',
 * and put them into the shared memory, using python to calculate,
 * and got prediction through member function 'Get[xxx]()'.
 */
class CQIDL : public Object
{
  public:
    CQIDL();
    ~CQIDL() override;
    static TypeId GetTypeId();

    void SetWbCQI(uint8_t cqi);
    uint8_t GetWbCQI();
};

} // namespace ns3
