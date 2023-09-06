/*
 * Copyright (c) 2018 Piotr Gawlowicz
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
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 * Modify: Muyuan Shen <muyuan_shen@hust.edu.cn>
 *
 */

#ifndef OPENGYM_ENV_H
#define OPENGYM_ENV_H

#include "ns3/object.h"

namespace ns3
{

class OpenGymSpace;
class OpenGymDataContainer;
class OpenGymInterface;

class OpenGymEnv : public Object
{
  public:
    OpenGymEnv();
    ~OpenGymEnv() override;

    static TypeId GetTypeId();

    virtual Ptr<OpenGymSpace> GetActionSpace() = 0;
    virtual Ptr<OpenGymSpace> GetObservationSpace() = 0;
    virtual bool GetGameOver() = 0;
    virtual Ptr<OpenGymDataContainer> GetObservation() = 0;
    virtual float GetReward() = 0;
    virtual std::string GetExtraInfo() = 0;
    virtual bool ExecuteActions(Ptr<OpenGymDataContainer> action) = 0;

    void SetOpenGymInterface(Ptr<OpenGymInterface> openGymInterface);
    void Notify();
    void NotifySimulationEnd();

  protected:
    // Inherited
    void DoInitialize() override;
    void DoDispose() override;

    Ptr<OpenGymInterface> m_openGymInterface;

  private:
};

} // end of namespace ns3

#endif /* OPENGYM_ENV_H */
