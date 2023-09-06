/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
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
 * Author:  Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#include <ns3/ai-module.h>
#include <ns3/core-module.h>

#include <chrono>
#include <iostream>
#include <random>

#define NUM_ENV 10000

namespace ns3
{

class ApbEnv : public OpenGymEnv
{
  public:
    ApbEnv();
    ~ApbEnv() override;
    static TypeId GetTypeId();
    void DoDispose() override;

    uint32_t GetAPlusB();

    // OpenGym interfaces:
    Ptr<OpenGymSpace> GetActionSpace() override;
    Ptr<OpenGymSpace> GetObservationSpace() override;
    bool GetGameOver() override;
    Ptr<OpenGymDataContainer> GetObservation() override;
    float GetReward() override;
    std::string GetExtraInfo() override;
    bool ExecuteActions(Ptr<OpenGymDataContainer> action) override;

    uint32_t m_a;
    uint32_t m_b;

  private:
    uint32_t m_sum;
};

ApbEnv::ApbEnv()
{
    SetOpenGymInterface(OpenGymInterface::Get());
}

ApbEnv::~ApbEnv()
{
}

TypeId
ApbEnv::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ApbEnv").SetParent<OpenGymEnv>().SetGroupName("OpenGym");
    return tid;
}

void
ApbEnv::DoDispose()
{
}

uint32_t
ApbEnv::GetAPlusB()
{
    Notify();
    return m_sum;
}

Ptr<OpenGymSpace>
ApbEnv::GetActionSpace()
{
    std::vector<uint32_t> shape = {1};
    std::string dtype = TypeNameGet<uint32_t>();
    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(0, 20, shape, dtype);
    return box;
}

Ptr<OpenGymSpace>
ApbEnv::GetObservationSpace()
{
    std::vector<uint32_t> shape = {2};
    std::string dtype = TypeNameGet<uint32_t>();
    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(0, 10, shape, dtype);
    return box;
}

bool
ApbEnv::GetGameOver()
{
    return false;
}

Ptr<OpenGymDataContainer>
ApbEnv::GetObservation()
{
    std::vector<uint32_t> shape = {2};
    Ptr<OpenGymBoxContainer<uint32_t>> box = CreateObject<OpenGymBoxContainer<uint32_t>>(shape);

    box->AddValue(m_a);
    box->AddValue(m_b);

    return box;
}

float
ApbEnv::GetReward()
{
    return 0.0;
}

std::string
ApbEnv::GetExtraInfo()
{
    return "";
}

bool
ApbEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
    Ptr<OpenGymBoxContainer<uint32_t>> box = DynamicCast<OpenGymBoxContainer<uint32_t>>(action);
    m_sum = box->GetValue(0);
    return true;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
    using namespace ns3;

    Ptr<ApbEnv> apb = CreateObject<ApbEnv>();

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distrib(1, 10);

    uint32_t sum;

    for (int i = 0; i < NUM_ENV; ++i)
    {
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
