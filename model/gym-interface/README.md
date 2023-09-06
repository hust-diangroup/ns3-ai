# Gym Interface

## Introduction

The Gym interface of ns3-ai transforms ns-3 into a RL playground. With this interface, high-level
observations (states), actions, rewards and game status information can be exchanged
between C++ and Python.

## Tutorial

The following tutorial of this interface is based on the [A-Plus-B](../../examples/a-plus-b) example.

### C++ side

The C++ side of Gym interface provides skeleton code to create
[Gymnasium](https://gymnasium.farama.org/index.html)-compatible environment at ns-3,
and a callback-based mechanism to easily collect information and execute actions.

To begin, simply inherit from `OpenGymEnv` to create an environment. In the code below,
method `GetAPlusB`, variables `m_a`, `m_b` and `m_sum` are added to the base class.

```c++
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
```

C++ side sets the numbers and gets their sum from Python. This is done by `GetAPlusB()`:

```c++
uint32_t
ApbEnv::GetAPlusB()
{
    Notify();
    return m_sum;
}
```

The `Notify()` function, defined in base class, is the core of C++-Python interaction. It registers essential callbacks,
collects state and send it to Python, receives the action, and executes it.
In the A Plus B example, the execution of action is simply storing the sum in `m_sum`. So, after `Notify()`, `m_sum` becomes the sum
of `m_a` and `m_b`.

In order for `Notify()` to work normally, some methods (used as callbacks) must be implemented:
1. `GetActionSpace`: Called when initialing Gym interface. It defines the action space of environment. In this example, action space
   contains one integer between 0 and 20, so box space is applied:
```c++
Ptr<OpenGymSpace>
ApbEnv::GetActionSpace()
{
    std::vector<uint32_t> shape = {1};
    std::string dtype = TypeNameGet<uint32_t>();
    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(0, 20, shape, dtype);
    return box;
}
```
2. `GetObservationSpace`: Similar to `GetActionSpace`, it defines the observation space of environment, which contains two integer between 0 and 10:
```c++
Ptr<OpenGymSpace>
ApbEnv::GetObservationSpace()
{
    std::vector<uint32_t> shape = {2};
    std::string dtype = TypeNameGet<uint32_t>();
    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(0, 10, shape, dtype);
    return box;
}
```
3. `GetGameOver`: In Gym interface an environment have two ways to stop: game over or simulation end. In this example we
   prefer the latter to stop the environment. Therefore, the return value is always `false`:
```c++
bool
ApbEnv::GetGameOver()
{
    return false;
}
```
4. `GetObservation`: Function to collect observation (state) from environment. In this example, `m_a` and `m_b` are collected.
   Note that `OpenGymBoxContainer` rather than `OpenGymBoxSpace` (in space definition) is used.
```c++
Ptr<OpenGymDataContainer>
ApbEnv::GetObservation()
{
    std::vector<uint32_t> shape = {2};
    Ptr<OpenGymBoxContainer<uint32_t>> box = CreateObject<OpenGymBoxContainer<uint32_t>>(shape);

    box->AddValue(m_a);
    box->AddValue(m_b);

    return box;
}
```
5. `GetReward`: Function that define the reward (`float` type). Reward is unused in A Plus B, can be arbitrary:
```c++
float
ApbEnv::GetReward()
{
    return 0.0;
}
```
6. `GetExtraInfo`: Function that pass additional info to Python. Info is also unused, thus it is an empty value:
```c++
std::string
ApbEnv::GetExtraInfo()
{
    return "";
}
```
7. `ExecuteActions`: Function that executes the action according to the information in container. In this example, we get the
   sum of a + b from the zeroth (and the only) item of action:
```c++
bool
ApbEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
    Ptr<OpenGymBoxContainer<uint32_t>> box = DynamicCast<OpenGymBoxContainer<uint32_t>>(action);
    m_sum = box->GetValue(0);
    return true;
}
```

After those functions defined, the `ApbEnv` class can be created and used.

```c++
using namespace ns3;
Ptr<ApbEnv> apb = CreateObject<ApbEnv>();
```

And the interaction is seamless:

```c++
std::cout << "set: " << apb->m_a << "," << apb->m_b << ";";
std::cout << "\n";

sum = apb->GetAPlusB();

std::cout << "get: " << sum << ";";
std::cout << "\n";
```

Remember to call `NotifySimulationEnd` before exit, to properly destroy the interface:

```c++
apb->NotifySimulationEnd();
```

### Python side

You don't need to write much code on Python side, because the
internal of the interface already implements [Gymnasium](https://gymnasium.farama.org/index.html)
APIs such as `reset`, `step` and `close`.

Start by importing essential modules:

```python
import ns3ai_gym_env
import gymnasium as gym
```

While your IDE may warn that module `ns3ai_gym_env` is unused, it's necessary to `import ns3ai_gym_env`
as it registers the `ns3ai_gym_env/Ns3-v0` environment in gym.

Define the agent that interacts with ns-3 environment:

```python
class ApbAgent:

    def __init__(self):
        pass

    def get_action(self, obs, reward, done, info):

        a = obs[0]
        b = obs[1]
        act = a + b

        return [act]
```

The `get_action` is used for summing a & b (observation), and returns an array containing the sum (action).

Create the environment and do initial setup:

```python
env = gym.make("ns3ai_gym_env/Ns3-v0")
ob_space = env.observation_space
ac_space = env.action_space
print("Observation space: ", ob_space, ob_space.dtype)
print("Action space: ", ac_space, ac_space.dtype)
obs, info = env.reset()
reward = 0
done = False
agent = ApbAgent()
```

Interact with C++ side:

```python
while True:

    action = agent.get_action(obs, reward, info, done)
    # print("---action: ", action)

    obs, reward, done, _, info = env.step(action)
    # print("---obs, reward, done, info: ", obs, reward, done, info)

    if done:
        break
```

When C++ side calls `NotifySimulationEnd`, `done` becomes true and Python exits the loop.

Remember to close the environment before exit:

```python
env.close()
```
