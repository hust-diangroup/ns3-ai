#include "ns3/core-module.h"
#include "ns3/ns3-ai-module.h"
#include "ns3/log.h"

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("multi-run");

/**
 * \brief Shared memory to store a and b.
 *
 * This struct is the environment (in this example, contain 'a' and 'b')
 * shared between ns-3 and python with the same shared memory
 * using the ns3-ai model.
 */
struct Env
{
    int a;
    int b;
}Packed;

/**
 * \brief Shared memory to store action c.
 *
 * This struct is the result (in this example, contain 'c')
 * calculated by python and put back to ns-3 with the shared memory.
 */
struct Act
{
    int c;
}Packed;

/**
 * \brief A class to calculate APB (a plus b).
 *
 * This class shared memory with python by the same id,
 * and got two variable a and b, and then put them into the shared memory
 * using python to calculate c=a+b, and got c from python.
 */
class APB : public Ns3AIRL<Env, Act>
{
public:
    APB(uint16_t id);
    int Func(int a, int b);
};

/**
 * \brief Link the shared memory with the id and set the operation lock
 *
 * \param[in] id  shared memory id, should be the same in python and ns-3
 */
APB::APB(uint16_t id) : Ns3AIRL<Env, Act>(id) {
    SetCond(2, 0);      ///< Set the operation lock (even for ns-3 and odd for python).
}

/**
 * \param[in] a  a number to be added.
 *
 * \param[in] b  another number to be added.
 *
 * \returns the result of a+b.
 *
 * put a and b into the shared memory;
 * wait for the python to calculate the result c = a + b;
 * get the result c from shared memory;
 */
int APB::Func(int a, int b)
{
    auto env = EnvSetterCond();     ///< Acquire the Env memory for writing
    env->a = a;
    env->b = b;
    SetCompleted();                 ///< Release the memory and update conters
    NS_LOG_DEBUG ("Ver:" << (int)SharedMemoryPool::Get()->GetMemoryVersion(m_id));
    auto act = ActionGetterCond();  ///< Acquire the Act memory for reading
    int ret = act->c;
    GetCompleted();                 ///< Release the memory, roll back memory version and update conters
    NS_LOG_DEBUG ("Ver:" << (int)SharedMemoryPool::Get()->GetMemoryVersion(m_id));
    return ret;
}

int main(int argc, char *argv[])
{
    int memblock_key = 2333;        ///< memory block key, need to keep the same in the python script
    int a = 1;
    int b = 2;
    CommandLine cmd;
    cmd.AddValue ("a","the value of a",a);
    cmd.AddValue ("b","the value of b",b);
    cmd.AddValue ("key","memory block key",memblock_key);
    cmd.Parse (argc, argv);
    APB apb(memblock_key);
    std::cout << a << "+" << b << "=" << apb.Func(a, b) << std::endl;
    apb.SetFinish();
    return 0;
}