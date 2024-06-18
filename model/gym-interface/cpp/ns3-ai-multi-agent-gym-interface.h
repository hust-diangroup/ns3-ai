#ifndef NS3_AI_MULTI_AGENT_GYM_INTERFACE_H
#define NS3_AI_MULTI_AGENT_GYM_INTERFACE_H

#include "../ns3-ai-gym-msg.h"

#include <ns3/ai-module.h>
#include <ns3/callback.h>
#include <ns3/core-module.h>
#include <ns3/object.h>
#include <ns3/ptr.h>
#include <ns3/type-id.h>

namespace ns3
{

class OpenGymSpace;
class OpenGymDataContainer;
class OpenGymEnv;

class OpenGymMultiAgentInterface : public Singleton<OpenGymMultiAgentInterface>, public Object
{
  public:
    OpenGymMultiAgentInterface();
    ~OpenGymMultiAgentInterface() override;
    static TypeId GetTypeId();

    void Init();
    void NotifyCurrentState(const std::string agentId,
                            Ptr<OpenGymDataContainer> obsDataContainer,
                            float reward,
                            bool isGameOver,
                            const std::map<std::string, std::string>& extraInfo,
                            Time actionDelay,
                            Callback<void, Ptr<OpenGymDataContainer>> actionCallback);
    void WaitForStop(float reward,
                     bool isGameOver,
                     const std::map<std::string, std::string>& extraInfo = {});
    void NotifySimulationEnd(float reward = 0,
                             const std::map<std::string, std::string>& extraInfo = {});

    std::map<std::string, Ptr<OpenGymSpace>> GetActionSpace();
    std::map<std::string, Ptr<OpenGymSpace>> GetObservationSpace();

    void SetGetActionSpaceCb(std::string agentId, Callback<Ptr<OpenGymSpace>> cb);
    void SetGetObservationSpaceCb(std::string agentId, Callback<Ptr<OpenGymSpace>> cb);

  private:

    bool m_simEnd;
    bool m_stopEnvRequested;
    bool m_initSimMsgSent;

    std::map<std::string, Callback<Ptr<OpenGymSpace>>> m_actionSpaceCbs;
    std::map<std::string, Callback<Ptr<OpenGymSpace>>> m_observationSpaceCbs;
};

} // end of namespace ns3

#endif // NS3_AI_MULTI_AGENT_GYM_INTERFACE_H
