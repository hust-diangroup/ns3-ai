/*
 * Copyright (c) 2023 University of Washington
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
 */

#ifndef TGAX_RESIDENTIAL_PROPAGATION_LOSS_MODEL_H_
#define TGAX_RESIDENTIAL_PROPAGATION_LOSS_MODEL_H_

#include "ns3/propagation-loss-model.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

class MobilityModel;

/**
 * \ingroup wifi
 *
 * This model implements the pathloss and shadowing models of the residential indoor scenario
 * described in IEEE 802.11-14/0980r16.  Fast fading (according to TGac channel model D)
 * is out-of-scope and should be handled by an error model.
 *
 * TODO:  Document the pathloss equation
 *
 * This model is buildings-aware.  If buildings are in the scenario and each node involved in
 * the signal has a MobilityBuildingInfo aggregated to the MobilityModel, then this model will
 * use the building information to compute the pathloss.  If the two nodes in question are
 * not within the same building, this model will currently block propagation by returning a
 * value of zero from DoCalcRxPower().  If neither node involed in the signal has a
 * MobilityBuildingInfo, this model will assume that both nodes are within the same room of
 * a building.
 *
 * Shadowing is a zero-mean Gaussian random variable; the value will be set upon first use
 * and will persist until at least one of the nodes changes position, in which case a new
 * value will be drawn (with no spatial correlation).
 *
 * Although this class is buildings-aware, it does not derive from BuildingsPropagationLossModel,
 * nor does it use a ChannelConditionModel-- both of those classes are more specific to
 * cellular/outdoor use cases.  It makes use of MobilityBuildingInfo from the buildings module.
 *
 * Objects of this type may be added to either a YansWifiChannel or a SpectrumChannel.
 */
class TgaxResidentialPropagationLossModel : public PropagationLossModel
{
  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    TgaxResidentialPropagationLossModel();
    // function to calculate rxPower
    double GetRxPower(double txPowerDbm, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

  protected:
    // override from PropagationLossModel
    double DoCalcRxPower(double txPowerDbm,
                         Ptr<MobilityModel> a,
                         Ptr<MobilityModel> b) const override;
    // override from PropagationLossModel
    int64_t DoAssignStreams(int64_t stream) override;

  private:
    double m_frequencyHz;    //!< frequency, in Hz
    double m_shadowingSigma; //!< sigma (dB) for shadowing std. deviation
    Ptr<NormalRandomVariable>
        m_shadowingRandomVariable; //!< random variable used for shadowing loss
};

} // namespace ns3

#endif /* TGAX_RESIDENTIAL_PROPAGATION_LOSS_MODEL_H_ */
