#include "ns3/core-module.h"
#include "ns3/ns3-ai-module.h"

using namespace std;
using namespace ns3;
struct Env
{
    int a;
    int b;
}Packed;
struct Act
{
    int c;
}Packed;

class APB : public Ns3AIRL<Env, Act>
{
public:
    APB(uint16_t id);
    int Func(int a, int b);
};
APB::APB(uint16_t id) : Ns3AIRL<Env, Act>(id) { SetCond(2, 0); }
int APB::Func(int a, int b)
{
    auto env = EnvSetterCond();
    env->a = a;
    env->b = b;
    SetCompleted();
    // std::cerr<<"Ver:"<<(int)SharedMemoryPool::Get()->GetMemoryVersion(m_id)<<std::endl;
    auto act = ActionGetterCond();
    int ret = act->c;
    GetCompleted();
    // std::cerr<<"Ver:"<<(int)SharedMemoryPool::Get()->GetMemoryVersion(m_id)<<std::endl;
    return ret;
}
int main()
{
    APB apb(2333);
    std::cout << apb.Func(1, 2) << std::endl;
    apb.SetFinish();
    return 0;
}