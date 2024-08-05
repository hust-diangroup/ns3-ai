#include <ns3/ai-module.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace ns3;

class Agent
{
  public:
    Agent(){};

    Agent(const std::string id, int number, Time stepTime)
        : m_id(id),
          m_number(number),
          m_stepTime(stepTime)
    {
    }

    ~Agent()
    {
    }

    void ExecuteAction(Ptr<OpenGymDataContainer> action)
    {
        // actions that are passed to the agent by the interface are abstract
        // OpenGymDataContainer objects and need to be transformed to the actual object type that
        // corresponds to the action space of the agent
        m_number += DynamicCast<OpenGymDiscreteContainer>(action)->GetValue() - 5;
    }

    Ptr<OpenGymDataContainer> GetObservation() const
    {
        auto shape = std::vector<uint32_t>{1};
        auto observation = CreateObject<OpenGymBoxContainer<int>>(
            shape); // Create a 1-dimensional
                    // container that holds the agents observation
        observation->AddValue(m_number);
        return observation;
    }

    double GetReward() const
    {
        return -abs(m_number); // The goal of the agent is it to reach the number 0
    }

    Ptr<OpenGymSpace> GetObservationSpace()
    {
        auto type = TypeNameGet<int>();
        auto shape = std::vector<uint32_t>{1};
        auto obsSpace = CreateObject<OpenGymBoxSpace>(-INFINITY, INFINITY, shape, type);
        return obsSpace;
    }

    Ptr<OpenGymSpace> GetActionSpace()
    {
        auto actionSpace = CreateObject<OpenGymDiscreteSpace>(10);
        return actionSpace;
    }

    void Step()
    {
        OpenGymMultiAgentInterface::Get()->NotifyCurrentState(
            m_id,
            GetObservation(),
            GetReward(),
            false,
            {},
            Seconds(0),
            MakeCallback(&Agent::ExecuteAction, this));
        Simulator::Schedule(m_stepTime, &Agent::Step, this);
    }

  private:
    const std::string m_id;
    int m_number;
    Time m_stepTime;
};

int
main(int argc, char* argv[])
{
    int numAgents = 2;
    int seedRunNumber = 1;
    CommandLine cmd;
    cmd.AddValue("numAgents", "Number of agents that act in the environment", numAgents);
    cmd.AddValue("seedRunNumber",
                 "Counts how often the environment has been reset (used for seeding)",
                 seedRunNumber);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(42);
    RngSeedManager::SetRun(seedRunNumber);

    auto randomNumber = CreateObject<UniformRandomVariable>();
    randomNumber->SetAttribute("Min", DoubleValue(-42));
    randomNumber->SetAttribute("Max", DoubleValue(42));

    std::vector<Agent*> agents;
    for (int i = 0; i < numAgents; i++)
    {
        std::string id = "agent_" + std::to_string(i);
        int number = randomNumber->GetInteger();
        Time stepTime = Seconds(1);
        auto agent = new Agent(id, number, stepTime);
        agents.emplace_back(agent);

        OpenGymMultiAgentInterface::Get()->SetGetObservationSpaceCb(
            id,
            MakeCallback(&Agent::GetObservationSpace, agents[i]));
        OpenGymMultiAgentInterface::Get()->SetGetActionSpaceCb(
            id,
            MakeCallback(&Agent::GetActionSpace, agents[i]));
    }

    for (const auto agent : agents)
    {
        Simulator::Schedule(Seconds(0), &Agent::Step, agent);
    }

    Simulator::Stop(Seconds(60));
    Simulator::Run();
    Simulator::Destroy();
    OpenGymMultiAgentInterface::Get()->NotifySimulationEnd(-100, {});
}
