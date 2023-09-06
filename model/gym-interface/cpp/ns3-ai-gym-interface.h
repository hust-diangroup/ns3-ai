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

#ifndef NS3_NS3_AI_GYM_INTERFACE_H
#define NS3_NS3_AI_GYM_INTERFACE_H

#include "../ns3-ai-gym-msg.h"

#include <ns3/ai-module.h>
#include <ns3/callback.h>
#include <ns3/object.h>
#include <ns3/ptr.h>
#include <ns3/type-id.h>

namespace ns3
{

class OpenGymSpace;
class OpenGymDataContainer;
class OpenGymEnv;

class OpenGymInterface : public Object
{
  public:
    static Ptr<OpenGymInterface> Get();
    OpenGymInterface();
    ~OpenGymInterface() override;
    static TypeId GetTypeId();

    void Init();
    void NotifyCurrentState();
    void WaitForStop();
    void NotifySimulationEnd();

    Ptr<OpenGymSpace> GetActionSpace();
    Ptr<OpenGymSpace> GetObservationSpace();
    Ptr<OpenGymDataContainer> GetObservation();
    float GetReward();
    bool IsGameOver();
    std::string GetExtraInfo();
    bool ExecuteActions(Ptr<OpenGymDataContainer> action);

    void SetGetActionSpaceCb(Callback<Ptr<OpenGymSpace>> cb);
    void SetGetObservationSpaceCb(Callback<Ptr<OpenGymSpace>> cb);
    void SetGetObservationCb(Callback<Ptr<OpenGymDataContainer>> cb);
    void SetGetRewardCb(Callback<float> cb);
    void SetGetGameOverCb(Callback<bool> cb);
    void SetGetExtraInfoCb(Callback<std::string> cb);
    void SetExecuteActionsCb(Callback<bool, Ptr<OpenGymDataContainer>> cb);

    void Notify(Ptr<OpenGymEnv> entity);

  protected:
    // Inherited
    void DoInitialize() override;
    void DoDispose() override;

  private:
    static Ptr<OpenGymInterface>* DoGet();
    //    static void Delete();

    bool m_simEnd;
    bool m_stopEnvRequested;
    bool m_initSimMsgSent;

    Callback<Ptr<OpenGymSpace>> m_actionSpaceCb;
    Callback<Ptr<OpenGymSpace>> m_observationSpaceCb;
    Callback<bool> m_gameOverCb;
    Callback<Ptr<OpenGymDataContainer>> m_obsCb;
    Callback<float> m_rewardCb;
    Callback<std::string> m_extraInfoCb;
    Callback<bool, Ptr<OpenGymDataContainer>> m_actionCb;
};

} // end of namespace ns3

#endif // NS3_NS3_AI_GYM_INTERFACE_H
