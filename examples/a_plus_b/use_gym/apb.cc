#include <iostream>
#include <random>
#include <chrono>
#include <ns3/core-module.h>
#include <ns3/ns3-ai-module.h>

#define NUM_ENV 10000

namespace ns3
{

class ApbEnv : public OpenGymEnv
{
  public:
    ApbEnv()
    {
        SetOpenGymInterface(OpenGymInterface::Get());
    }

    ~ApbEnv() override
    {
    }

    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::ApbEnv").SetParent<OpenGymEnv>().SetGroupName("OpenGym");
        return tid;
    }

    void DoDispose() override
    {
    }

    uint32_t GetAPlusB()
    {
        Notify();
        return m_sum;
    }

    // OpenGym interfaces:
    Ptr<OpenGymSpace> GetActionSpace() override
    {
        std::vector<uint32_t> shape = {1};
        std::string dtype = TypeNameGet<uint32_t>();
        Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(0, 20, shape, dtype);
        return box;
    }

    Ptr<OpenGymSpace> GetObservationSpace() override
    {
        std::vector<uint32_t> shape = {2};
        std::string dtype = TypeNameGet<uint32_t>();
        Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(0, 10, shape, dtype);
        return box;
    }

    bool GetGameOver() override
    {
        return false;
    }

    Ptr<OpenGymDataContainer> GetObservation() override
    {
        std::vector<uint32_t> shape = {2};
        Ptr<OpenGymBoxContainer<uint32_t>> box = CreateObject<OpenGymBoxContainer<uint32_t>>(shape);

        box->AddValue(m_a);
        box->AddValue(m_b);

        return box;
    }

    float GetReward() override
    {
        return 0.0;
    }

    std::string GetExtraInfo() override
    {
        return "";
    }

    bool ExecuteActions(Ptr<OpenGymDataContainer> action) override
    {
        Ptr<OpenGymBoxContainer<uint32_t>> box = DynamicCast<OpenGymBoxContainer<uint32_t>>(action);
        m_sum = box->GetValue(0) + box->GetValue(1);
        return true;
    }

    uint32_t m_a;
    uint32_t m_b;

  private:
    uint32_t m_sum;
};

}

int main(int argc, char *argv[])
{
    using namespace ns3;

    Ptr<ApbEnv> apb = CreateObject<ApbEnv>();

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    uint32_t sum;

    for (int i = 0; i < NUM_ENV; ++i) {

        apb->m_a = distrib(gen);
        apb->m_b = distrib(gen);
        std::cout << "set: " << apb->m_a << "," << apb->m_b << ";";
        std::cout << "\n";

        sum = apb->GetAPlusB();

        std::cout << "get: " << sum << ";";
        std::cout << "\n";

    }

    apb->NotifySimulationEnd();

    return 0;
}
