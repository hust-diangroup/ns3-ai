#include "ns3-ai-multi-agent-gym-interface.h"

#include "container.h"
#include "messages.pb.h"
#include "ns3-ai-gym-env.h"
#include "spaces.h"

#include <ns3/config.h>
#include <ns3/log.h>
#include <ns3/simulator.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OpenGymMultiAgentInterface");
NS_OBJECT_ENSURE_REGISTERED(OpenGymMultiAgentInterface);

OpenGymMultiAgentInterface::OpenGymMultiAgentInterface()
    : m_simEnd(false),
      m_stopEnvRequested(false),
      m_initSimMsgSent(false)
{
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(false);
    interface->SetHandleFinish(false);
}

OpenGymMultiAgentInterface::~OpenGymMultiAgentInterface()
{
}

TypeId
OpenGymMultiAgentInterface::GetTypeId()
{
    static TypeId tid = TypeId("OpenGymMultiAgentInterface")
                            .SetParent<Object>()
                            .SetGroupName("OpenGym")
                            .AddConstructor<OpenGymMultiAgentInterface>();
    return tid;
}

void
OpenGymMultiAgentInterface::Init()
{
    // do not send init msg twice
    if (m_initSimMsgSent)
    {
        return;
    }
    m_initSimMsgSent = true;

    ns3_ai_gym::MultiAgentSimInitMsg simInitMsg;

    // obs space
    for (const auto& [key, value] : GetObservationSpace())
    {
        (*simInitMsg.mutable_obsspaces())[key] = value->GetSpaceDescription();
    }

    // action space
    for (const auto& [key, value] : GetActionSpace())
    {
        (*simInitMsg.mutable_actspaces())[key] = value->GetSpaceDescription();
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
OpenGymMultiAgentInterface::NotifyCurrentState(
    const std::string agentId,
    Ptr<OpenGymDataContainer> obsDataContainer,
    float reward,
    bool isGameOver,
    const std::map<std::string, std::string>& extraInfo,
    Time actionDelay,
    Callback<void, Ptr<OpenGymDataContainer>> actionCallback)
{
    if (!m_initSimMsgSent)
    {
        Init();
    }
    if (m_stopEnvRequested)
    {
        return;
    }
    ns3_ai_gym::MultiAgentEnvStateMsg envStateMsg;
    // observation
    ns3_ai_gym::DataContainer obsDataContainerPbMsg;
    if (obsDataContainer)
    {
        obsDataContainerPbMsg = obsDataContainer->GetDataContainerPbMsg();
        envStateMsg.mutable_obsdata()->CopyFrom(obsDataContainerPbMsg);
    }
    // agent
    envStateMsg.set_agentid(agentId);
    // reward
    envStateMsg.set_reward(reward);
    // game over
    envStateMsg.set_isgameover(false);
    if (isGameOver)
    {
        envStateMsg.set_isgameover(true);
        if (m_simEnd)
        {
            envStateMsg.set_reason(ns3_ai_gym::MultiAgentEnvStateMsg::SimulationEnd);
        }
        else
        {
            envStateMsg.set_reason(ns3_ai_gym::MultiAgentEnvStateMsg::GameOver);
        }
    }
    // extra info
    for (const auto& [key, value] : extraInfo)
    {
        (*envStateMsg.mutable_info())[key] = value;
    }

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
        return;
    }

    bool stopSim = envActMsg.stopsimreq();
    if (stopSim)
    {
        NS_LOG_DEBUG("---Stop requested: " << stopSim);
        m_stopEnvRequested = true;
        Simulator::Stop();
        Simulator::Destroy();
        NS_ABORT_MSG("Simulation stopped!");
    }

    // first step after reset is called without actions, just to get current state
    ns3_ai_gym::DataContainer actDataContainerPbMsg = envActMsg.actdata();
    auto action = OpenGymDataContainer::CreateFromDataContainerPbMsg(actDataContainerPbMsg);
    Simulator::Schedule(actionDelay, actionCallback.Bind(action));
}

void
OpenGymMultiAgentInterface::WaitForStop(float reward,
                                        bool isGameOver,
                                        const std::map<std::string, std::string>& extraInfo)
{
    NS_LOG_FUNCTION(this);

    NotifyCurrentState(
        "",
        {},
        reward,
        isGameOver,
        extraInfo,
        Seconds(0),
        *[](Ptr<OpenGymDataContainer>) {});
}

void
OpenGymMultiAgentInterface::NotifySimulationEnd(float reward,
                                                const std::map<std::string, std::string>& extraInfo)
{
    NS_LOG_FUNCTION(this);
    m_simEnd = true;
    if (m_initSimMsgSent)
    {
        WaitForStop(reward, true, extraInfo);
    }
}

std::map<std::string, Ptr<OpenGymSpace>>
OpenGymMultiAgentInterface::GetActionSpace()
{
    NS_LOG_FUNCTION(this);
    std::map<std::string, Ptr<OpenGymSpace>> actionSpace;
    for (const auto& [agentId, callback] : m_actionSpaceCbs)
    {
        actionSpace[agentId] = callback();
    }
    return actionSpace;
}

std::map<std::string, Ptr<OpenGymSpace>>
OpenGymMultiAgentInterface::GetObservationSpace()
{
    NS_LOG_FUNCTION(this);
    std::map<std::string, Ptr<OpenGymSpace>> obsSpace;
    for (const auto& [agentId, callback] : m_observationSpaceCbs)
    {
        obsSpace[agentId] = callback();
    }
    return obsSpace;
}

void
OpenGymMultiAgentInterface::SetGetActionSpaceCb(std::string agentId, Callback<Ptr<OpenGymSpace>> cb)
{
    m_actionSpaceCbs[agentId] = cb;
}

void
OpenGymMultiAgentInterface::SetGetObservationSpaceCb(std::string agentId,
                                                     Callback<Ptr<OpenGymSpace>> cb)
{
    m_observationSpaceCbs[agentId] = cb;
}

} // namespace ns3
