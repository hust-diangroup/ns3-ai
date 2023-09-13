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

/*
 * Note: The Gym interface class is only for C++ side. Do not create Python binding
 *       for this interface.
 */

#include "ns3-ai-gym-interface.h"

#include "container.h"
#include "messages.pb.h"
#include "ns3-ai-gym-env.h"
#include "spaces.h"

#include <ns3/config.h>
#include <ns3/log.h>
#include <ns3/simulator.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OpenGymInterface");
NS_OBJECT_ENSURE_REGISTERED(OpenGymInterface);

Ptr<OpenGymInterface>
OpenGymInterface::Get()
{
    NS_LOG_FUNCTION_NOARGS();
    return *DoGet();
}

OpenGymInterface::OpenGymInterface()
    : m_simEnd(false),
      m_stopEnvRequested(false),
      m_initSimMsgSent(false)
{
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(false);
    interface->SetHandleFinish(false);
}

OpenGymInterface::~OpenGymInterface()
{
}

TypeId
OpenGymInterface::GetTypeId()
{
    static TypeId tid = TypeId("OpenGymInterface")
                            .SetParent<Object>()
                            .SetGroupName("OpenGym")
                            .AddConstructor<OpenGymInterface>();
    return tid;
}

void
OpenGymInterface::Init()
{
    // do not send init msg twice
    if (m_initSimMsgSent)
    {
        return;
    }
    m_initSimMsgSent = true;

    Ptr<OpenGymSpace> obsSpace = GetObservationSpace();
    Ptr<OpenGymSpace> actionSpace = GetActionSpace();

    ns3_ai_gym::SimInitMsg simInitMsg;
    if (obsSpace)
    {
        ns3_ai_gym::SpaceDescription spaceDesc;
        spaceDesc = obsSpace->GetSpaceDescription();
        simInitMsg.mutable_obsspace()->CopyFrom(spaceDesc);
    }
    if (actionSpace)
    {
        ns3_ai_gym::SpaceDescription spaceDesc;
        spaceDesc = actionSpace->GetSpaceDescription();
        simInitMsg.mutable_actspace()->CopyFrom(spaceDesc);
    }

    // get the interface
    Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>* msgInterface =
        Ns3AiMsgInterface::Get()->GetInterface<Ns3AiGymMsg, Ns3AiGymMsg>();

    // send init msg to python
    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->size = simInitMsg.ByteSizeLong();
    assert(msgInterface->GetCpp2PyStruct()->size <= MSG_BUFFER_SIZE);
    simInitMsg.SerializeToArray(msgInterface->GetCpp2PyStruct()->buffer,
                                msgInterface->GetCpp2PyStruct()->size);
    msgInterface->CppSendEnd();

    // receive init ack msg from python
    ns3_ai_gym::SimInitAck simInitAck;
    msgInterface->CppRecvBegin();
    simInitAck.ParseFromArray(msgInterface->GetPy2CppStruct()->buffer,
                              msgInterface->GetPy2CppStruct()->size);
    msgInterface->CppRecvEnd();

    bool done = simInitAck.done();
    NS_LOG_DEBUG("Sim Init Ack: " << done);
    bool stopSim = simInitAck.stopsimreq();
    if (stopSim)
    {
        NS_LOG_DEBUG("---Stop requested: " << stopSim);
        m_stopEnvRequested = true;
        Simulator::Stop();
        Simulator::Destroy();
        std::exit(0);
    }
}

void
OpenGymInterface::NotifyCurrentState()
{
    if (!m_initSimMsgSent)
    {
        Init();
    }
    if (m_stopEnvRequested)
    {
        return;
    }
    // collect current env state
    Ptr<OpenGymDataContainer> obsDataContainer = GetObservation();
    float reward = GetReward();
    bool isGameOver = IsGameOver();
    std::string extraInfo = GetExtraInfo();
    ns3_ai_gym::EnvStateMsg envStateMsg;
    // observation
    ns3_ai_gym::DataContainer obsDataContainerPbMsg;
    if (obsDataContainer)
    {
        obsDataContainerPbMsg = obsDataContainer->GetDataContainerPbMsg();
        envStateMsg.mutable_obsdata()->CopyFrom(obsDataContainerPbMsg);
    }
    // reward
    envStateMsg.set_reward(reward);
    // game over
    envStateMsg.set_isgameover(false);
    if (isGameOver)
    {
        envStateMsg.set_isgameover(true);
        if (m_simEnd)
        {
            envStateMsg.set_reason(ns3_ai_gym::EnvStateMsg::SimulationEnd);
        }
        else
        {
            envStateMsg.set_reason(ns3_ai_gym::EnvStateMsg::GameOver);
        }
    }
    // extra info
    envStateMsg.set_info(extraInfo);

    // get the interface
    Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>* msgInterface =
        Ns3AiMsgInterface::Get()->GetInterface<Ns3AiGymMsg, Ns3AiGymMsg>();

    // send env state msg to python
    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyStruct()->size = envStateMsg.ByteSizeLong();
    assert(msgInterface->GetCpp2PyStruct()->size <= MSG_BUFFER_SIZE);
    envStateMsg.SerializeToArray(msgInterface->GetCpp2PyStruct()->buffer,
                                 msgInterface->GetCpp2PyStruct()->size);

    msgInterface->CppSendEnd();

    // receive act msg from python
    ns3_ai_gym::EnvActMsg envActMsg;
    msgInterface->CppRecvBegin();

    envActMsg.ParseFromArray(msgInterface->GetPy2CppStruct()->buffer,
                             msgInterface->GetPy2CppStruct()->size);
    msgInterface->CppRecvEnd();

    if (m_simEnd)
    {
        // if sim end only rx msg and quit
        return;
    }

    bool stopSim = envActMsg.stopsimreq();
    if (stopSim)
    {
        NS_LOG_DEBUG("---Stop requested: " << stopSim);
        m_stopEnvRequested = true;
        Simulator::Stop();
        Simulator::Destroy();
        std::exit(0);
    }

    // first step after reset is called without actions, just to get current state
    ns3_ai_gym::DataContainer actDataContainerPbMsg = envActMsg.actdata();
    Ptr<OpenGymDataContainer> actDataContainer =
        OpenGymDataContainer::CreateFromDataContainerPbMsg(actDataContainerPbMsg);
    ExecuteActions(actDataContainer);
}

void
OpenGymInterface::WaitForStop()
{
    NS_LOG_FUNCTION(this);
    //    NS_LOG_UNCOND("Wait for stop message");
    NotifyCurrentState();
}

void
OpenGymInterface::NotifySimulationEnd()
{
    NS_LOG_FUNCTION(this);
    m_simEnd = true;
    if (m_initSimMsgSent)
    {
        WaitForStop();
    }
}

Ptr<OpenGymSpace>
OpenGymInterface::GetActionSpace()
{
    NS_LOG_FUNCTION(this);
    Ptr<OpenGymSpace> actionSpace;
    if (!m_actionSpaceCb.IsNull())
    {
        actionSpace = m_actionSpaceCb();
    }
    return actionSpace;
}

Ptr<OpenGymSpace>
OpenGymInterface::GetObservationSpace()
{
    NS_LOG_FUNCTION(this);
    Ptr<OpenGymSpace> obsSpace;
    if (!m_observationSpaceCb.IsNull())
    {
        obsSpace = m_observationSpaceCb();
    }
    return obsSpace;
}

Ptr<OpenGymDataContainer>
OpenGymInterface::GetObservation()
{
    NS_LOG_FUNCTION(this);
    Ptr<OpenGymDataContainer> obs;
    if (!m_obsCb.IsNull())
    {
        obs = m_obsCb();
    }
    return obs;
}

float
OpenGymInterface::GetReward()
{
    NS_LOG_FUNCTION(this);
    float reward = 0.0;
    if (!m_rewardCb.IsNull())
    {
        reward = m_rewardCb();
    }
    return reward;
}

bool
OpenGymInterface::IsGameOver()
{
    NS_LOG_FUNCTION(this);
    bool gameOver = false;
    if (!m_gameOverCb.IsNull())
    {
        gameOver = m_gameOverCb();
    }
    return (gameOver || m_simEnd);
}

std::string
OpenGymInterface::GetExtraInfo()
{
    NS_LOG_FUNCTION(this);
    std::string info;
    if (!m_extraInfoCb.IsNull())
    {
        info = m_extraInfoCb();
    }
    return info;
}

bool
OpenGymInterface::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
    NS_LOG_FUNCTION(this);
    bool reply = false;
    if (!m_actionCb.IsNull())
    {
        reply = m_actionCb(action);
    }
    return reply;
}

void
OpenGymInterface::SetGetActionSpaceCb(Callback<Ptr<OpenGymSpace>> cb)
{
    m_actionSpaceCb = cb;
}

void
OpenGymInterface::SetGetObservationSpaceCb(Callback<Ptr<OpenGymSpace>> cb)
{
    m_observationSpaceCb = cb;
}

void
OpenGymInterface::SetGetGameOverCb(Callback<bool> cb)
{
    m_gameOverCb = cb;
}

void
OpenGymInterface::SetGetObservationCb(Callback<Ptr<OpenGymDataContainer>> cb)
{
    m_obsCb = cb;
}

void
OpenGymInterface::SetGetRewardCb(Callback<float> cb)
{
    m_rewardCb = cb;
}

void
OpenGymInterface::SetGetExtraInfoCb(Callback<std::string> cb)
{
    m_extraInfoCb = cb;
}

void
OpenGymInterface::SetExecuteActionsCb(Callback<bool, Ptr<OpenGymDataContainer>> cb)
{
    m_actionCb = cb;
}

void
OpenGymInterface::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymInterface::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymInterface::Notify(Ptr<OpenGymEnv> entity)
{
    NS_LOG_FUNCTION(this);

    SetGetGameOverCb(MakeCallback(&OpenGymEnv::GetGameOver, entity));
    SetGetObservationCb(MakeCallback(&OpenGymEnv::GetObservation, entity));
    SetGetRewardCb(MakeCallback(&OpenGymEnv::GetReward, entity));
    SetGetExtraInfoCb(MakeCallback(&OpenGymEnv::GetExtraInfo, entity));
    SetExecuteActionsCb(MakeCallback(&OpenGymEnv::ExecuteActions, entity));

    NotifyCurrentState();
}

Ptr<OpenGymInterface>*
OpenGymInterface::DoGet()
{
    static Ptr<OpenGymInterface> ptr = CreateObject<OpenGymInterface>();
    return &ptr;
}

} // namespace ns3
