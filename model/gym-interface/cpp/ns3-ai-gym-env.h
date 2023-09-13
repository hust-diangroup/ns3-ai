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

#include <ns3/object.h>

namespace ns3
{

class OpenGymSpace;
class OpenGymDataContainer;
class OpenGymInterface;

/**
 * \brief Base class to create Gymnasium-compatible environments in C++ side.
 */
class OpenGymEnv : public Object
{
  public:
    OpenGymEnv();
    ~OpenGymEnv() override;

    static TypeId GetTypeId();

    /**
     * Get action space (Box, Dict, ...) from simulation
     */
    virtual Ptr<OpenGymSpace> GetActionSpace() = 0;

    /**
     * Get observation space (Box, Dict, ...) from simulation
     */
    virtual Ptr<OpenGymSpace> GetObservationSpace() = 0;

    /**
     * Get whether game is over (simulation is finished).
     * Normally, this should always return false.
     */
    virtual bool GetGameOver() = 0;

    /**
     * Get observation (stored in container)
     */
    virtual Ptr<OpenGymDataContainer> GetObservation() = 0;

    /**
     * Get reward
     */
    virtual float GetReward() = 0;

    /**
     * Get extra information
     */
    virtual std::string GetExtraInfo() = 0;

    /**
     * Execute actions. E.g., modify the contention window in TCP.
     */
    virtual bool ExecuteActions(Ptr<OpenGymDataContainer> action) = 0;

    /**
     * Sets the lower level gym interface (shared memory)
     * associated to the environment
     */
    void SetOpenGymInterface(Ptr<OpenGymInterface> openGymInterface);

    /**
     * Notify Python side about the states, and execute the actions
     */
    void Notify();

    /**
     * Notify Python side that the simulation has ended.
     */
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
