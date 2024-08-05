# Multi-Agent Reinforcement Learning

## Background
The `Ns3MultiAgentEnv` allows the user to
create a multi-agent Gymnasium environment from an ns3 simulation,
facilitating the `OpenGymMultiAgentInterface` for inter-process
communication. This environment can then be used to train the agents
using reinforcement learning algorithms. We assume the reader is already
familiar with the concepts of reinforcement learning, multi-agent
systems, and the ns-3 simulator.

## Usage Overview

The following steps have to be carried out to create a multi-agent environment for a specific experiment:
1.  Create an ns-3 simulation with the desired network topology and
    traffic.
2.  Define how each agent observes and acts within the environment.
3.  Specify when an agent performs its inference and training steps.
4.  Decide on termination criteria for the environment.
5.  Register the environment in a Python script where it can be used to
    interact with the ns3 simulation

Steps 1 to 4 require to write **C++** code utilizing the API of the
`OpenGymMultiAgentInterface`. Step 5 is done in **Python** by creating
an instance of the `Ns3MultiAgentEnv`. In the following sections, we
will guide the user through the usage of both of these components in
general and provide a minimal example to demonstrate the usage.

## Basic Example

For the scope of this documentation, we decided on the following example.
We will create a variable number of agents in our ns3 simulation. Each
of these agents will be instantiated with a random counter ranging from
-42 to +42. When doing inference, each agent can decide on a number
between -5 and +5. This number will be added to the counter of this
agent. The goal of each agent is to reach the counter value 0, therefore
the reward for each agent is the negative absolute value of its counter.
The agents infer once every second and the experiment is truncated at 60
seconds (simulation end). The agents are first evaluated with random
actions, then trained using the DQN algorithm and finally an evaluation
based on a checkpoint of the training is performed.

Because the agents behave very similarly we introduce the `Agent` class
in our ns3 script. The relevant methods this class provides will also be
discussed in the following sections. Overall it is not necessary to
create new classes in order to work with the
`OpenGymMultiAgentInterface` and we will also show how it can be used
without them.

## OpenGymMultiAgentInterface

In general, the `OpenGymMultiAgentInterface` is responsible for:
-   Registering agents with their corresponding observation and action
    spaces in the environment
-   Performing inference and training steps for a given agent
-   Terminating the environment and handling the simulation end

### Accessing the Interface

To use the `OpenGymMultiAgentInterface` in the ns3 simulation, the user
has to include the ns3ai-module via

``` cpp
#include <ns3/ai-module.h>
```

The interface is then provided as a singleton and can be used inside the
simulation without the need for instantiating it. The user can access
the interface via

``` cpp
OpenGymMultiAgentInterface::Get()
```

This returns a pointer to the interface from which the other methods can
be accessed.

### Registering Agents

To register an agent with the interface, the user has to provide the following information:
-   The agent's ID
-   The observation space of the agent
-   The action space of the agent

The **agent id** is an arbitrary string that is used to identify the
agent in the simulation and in the final Python environment. The
observation and action spaces are defined as `OpenGymSpaces` and
registered by providing callbacks, which return the space information.
The callbacks are then used in
`OpenGymMultiAgentInterface::SetGetObservationSpaceCb` and
`OpenGymMultiAgentInterface::SetGetActionSpaceCb` respectively.

The following code snippets demonstrate how to register the agents from
our example for the environment.

First, the observation and action spaces are defined in the agent class. In this simple example,
the observation is a single integer - the current number - while the possible action is from the discrete space [0, 10].
The action will later on be transformed to the range [-5, 5]. 

``` cpp
Ptr<OpenGymSpace>
Agent::GetObservationSpace()
{
    auto type = TypeNameGet<int>();
    auto shape = std::vector<uint32_t>{1};
    auto obsSpace = CreateObject<OpenGymBoxSpace>(-INFINITY, INFINITY, shape, type);
    return obsSpace;
}

Ptr<OpenGymSpace>
Agent::GetActionSpace()
{
    auto actionSpace = CreateObject<OpenGymDiscreteSpace>(10);
    return actionSpace;
}
```

Then the agents are instantiated and registered in the environment.

``` cpp
auto randomNumber = CreateObject<UniformRandomVariable>();
randomNumber->SetAttribute("Min", DoubleValue(-42));
randomNumber->SetAttribute("Max", DoubleValue(42));

std::vector<Agent*> agents;
for (int i = 0; i < numAgents; i++)
{
    // create an agent that will step once a second with its
    // counter initialized randomly and a given id
    std::string id = "agent_" + std::to_string(i);
    int number = randomNumber->GetInteger();
    Time stepTime = Seconds(1);
    auto agent = new Agent(id, number, stepTime);
    agents.emplace_back(agent);

    // register the newly created agent in the environment
    OpenGymMultiAgentInterface::Get()->SetGetObservationSpaceCb(
        id,
        MakeCallback(&Agent::GetObservationSpace, agents[i]));
    OpenGymMultiAgentInterface::Get()->SetGetActionSpaceCb(
        id,
        MakeCallback(&Agent::GetActionSpace, agents[i]));
}
```
>[!NOTE]
>In case the user does not want to create an extra class for the agents,
>the callbacks can also be provided as lambda functions.
>``` cpp
>for (int i = 0; i < numAgents; i++)
>{
>    std::string id = "agent_" + std::to_string(i);
>    OpenGymMultiAgentInterface::Get()->SetGetObservationSpaceCb(id, []() {
>        auto type = TypeNameGet<int>();
>        auto shape = std::vector<uint32_t>{1};
>        auto obsSpace = CreateObject<OpenGymBoxSpace>(-INFINITY, INFINITY, shape, type);
>        return obsSpace;
>    });
>    OpenGymMultiAgentInterface::Get()->SetGetActionSpaceCb(id, []() {
>        auto actionSpace = CreateObject<OpenGymDiscreteSpace>(10);
>        return actionSpace;
>    });
>}
>```

### Performing Inference

To let an agent perform inference the following information has to be provided:
-   ID of the agent
-   Observation the agent made
-   Reward signal the agent received after its previous action
-   Indication whether the agent reached a terminal state
-   Extra information that is not used for training but the user is
    interested in
-   Time that indicates how long the inference takes in the simulation
-   How the inferred action shall be applied in the simulation

Signaling that an agent performs inference is done via
`OpenGymMultiAgentInterface::NotifyCurrentState`. This method needs to
be scheduled during simulation time, whenever an agent should compute
its next action.

>[!NOTE]
>The design of the interface allows only one agent to perform inference
>per call of `NotifyCurrentState`. Still, this does not restrict two
>agents to perform inference at the exact same time in the simulation. To
>do so, the user simply needs to schedule two calls of
>`NotifyCurrentState` at the same simulation time, and provide the
>different arguments.

The following code snippets demonstrate how `NotifyCurrentState` can be
used to perform an agent step in our example.:

``` cpp
void
Agent::Step()
{
    OpenGymMultiAgentInterface::Get()->NotifyCurrentState(
        m_id,
        GetObservation(),
        GetReward(),
        false, // the agent does not have a terminal state
        {},
        Seconds(0), // we assume performing inference is instantaneous
        MakeCallback(&Agent::ExecuteAction, this));

    // We want the agents to step periodically at fixed intervals
    Simulator::Schedule(m_stepTime, &Agent::Step, this);
}
```

In the simulation, the step method now just needs to be invoked once for
each agent.

``` cpp
for (const auto agent : agents)
{
    Simulator::Schedule(Seconds(0), &Agent::Step, agent);
}
```
>[!NOTE]
>The methods `GetObservation`, `GetReward`, and `ExecuteAction` of the newly
>created agent class are not provided by the
>interface itself. Again, as already demonstrated for the registration of
>the agents, the user could also use lambda functions together with
>`NotifyCurrentState` or even pass the corresponding values directly.

>[!WARNING]
>As already mentioned the interface utilizes so-called spaces and
>containers to communicate the observations and actions of agents. The
>user needs to make sure that the observations are correctly wrapped
>inside such a container and match the space description. For the actions,
>the user must extract the action from the provided container (this also
>needs to match the action space description).
>The following code demonstrates how such an action would be extracted
>and executed in our example:
>``` cpp
>void
>Agent::ExecuteAction(Ptr<OpenGymDataContainer> action)
>{
>    // the action space in this case is a discrete container ranging from 0 to 10
>    // such a container contains exactly one value
>    auto raw_action = DynamicCast<OpenGymDiscreteContainer>(action)->GetValue();
>    // the agent is allowed to choose a number between -5 and 5
>    // to and add it to its internal counter
>    m_number += raw_action - 5;
>}
>```

### Terminating the Environment

The simulation of the environment can end due to two possible reasons:
1.  An agent reached its terminal state
2.  The simulation ended

As we have already seen, the user can signal that an agent reached its
terminal state by setting the corresponding flag in
`NotifyCurrentState`. When the method is called with this flag set to
true, the simulation will be stopped and destroyed and all agents will
be treated as having reached their terminal state.

To signal the simulation end to the environment, the user can call
`OpenGymMultiAgentInterface::NotifySimulationEnd`. As additional
arguments, a final reward and extra information can be provided. In
reinforcement learning, this corresponds to the truncation of the episode.

The following code snippet demonstrates how the simulation end can be
signaled:

``` cpp
Simulator::Stop(Seconds(60));
Simulator::Run();
Simulator::Destroy();
// finish the environment without giving an extra reward and
// without providing extra information
OpenGymMultiAgentInterface::Get()->NotifySimulationEnd(0, {});
```
>[!WARNING]
>The call to `NotifySimulationEnd` must be executed as the very last
>method in the simulation script as it will destroy the C++ process once
>the information has been passed to the Python environment.
>It is also advised to include it in every experiment because it ensures
>that the RL algorithms understand that the episode has been truncated
>when the simulation time is over.

### Conclusion

With all the previous sections it should have become clear that the
`OpenGymMultiAgentInterface` is a powerful tool to create multi-agent
environments for reinforcement learning experiments. The user can define
the agents, their observations and actions, and the simulation end
criteria in a flexible way. The interface is designed to be easily
integrated into existing ns3 simulations. The interface is what users
will interact with when designing the simulation part of their
environment in C++. The next sections will demonstrate how to use the
`Ns3MultiAgentEnv` to interact with the ns3 simulation from a Python
script.

Also, we want to emphasize that all of these interactions only require
the `OpenGymMultiAgentInterface`. Any additional classes or methods
(e.g. the custom `Agent` class) are optional and only necessary to
ensure a clean and structured simulation script without too much
redundancy.

>[!WARNING]
>A simulation script that contains the `OpenGymMultiAgentInterface` is
>not intended to run and will not run properly on its own. It is only a
>part of the environment that is used to interact with the Python script.
>When running the script on its own, the simulation will fail because calling
>methods of the interface will not generate a response.
>Therefore, a Python script is necessary to complete the environment and
>to run the simulation.

## Ns3MultiAgentEnv

The `Ns3MultiAgentEnv` is a Python class that is used to interact with
the ns3 simulation via the `OpenGymMultiAgentInterface`. It provides all
the abstractions of a Gymnasium environment (with slight modifications
to allow for multi-agent setup). Overall it provides the step(),
reset(), and close() methods that are necessary to interact with the
environment. Rendering is not supported in the base class as the
requirements for visualization highly depend on the underlying ns3
simulation that is experimented with.

The following sections will guide the user through the possible
interactions with the `Ns3MultiAgentEnv` and provide a minimal example
to demonstrate the usage.

### Creating an Environment Instance

The `Ns3MultiAgentEnv` can be understood as a wrapper around an ns3
simulation that interacts with the `OpenGymMultiAgentInterface`. To
create an instance of the environment the user has to provide the build
target that will be run as the environment and the root directory where
the ns3 files are located (this directory contains for example the src,
contrib and build folders as subdirectories).

Also, the user might want to pass additional arguments to the ns3
simulation. These arguments can be passed as a dictionary. In our
example, the number of agents is not fixed and therefore the user can
pass the number of agents as an argument.

The following code snippets demonstrate how an environment instance can
be created for our example.

Preparation of the ns3 simulation to accept additional arguments:

``` cpp
int main(int argc, char* argv[])
{
    int numAgents = 2;
    CommandLine cmd;
    cmd.AddValue("numAgents", "Number of agents that act in the environment", numAgents);
    cmd.Parse(argc, argv);
//...
```

Creation of the environment instance in the Python script:

``` python
import os
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv  # this import is necessary to register the environment

targetName = "ns3ai_multi-agent"
ns3Path = str(os.getenv("NS3_HOME")) # assuming this contains the path to the root directory of ns3
ns3Settings: dict[str] = {"numAgents": 3}

env:Ns3MultiAgentEnv = Ns3MultiAgentEnv(targetName=targetName, ns3Path=ns3Path, ns3Settings=ns3Settings)

# code that interacts with the environment
# ...

env.close() # this is necessary to free the resources of the environment
```

Instead of the name of the build target, the user can also directly
provide the path to the executable that should be run as the
environment.

>[!NOTE]
>It is advised to use build targets configured with optimized
>build-profile settings. This often results in significant training
>speedups. See the
>[ns3-documentation](https://www.nsnam.org/docs/tutorial/html/getting-started.html#build-profiles)
>for more information on build profiles.

### Interacting with the Environment

To interact with the environment, use the `reset()` and `step()` methods
from the Gymnasium standard. An experiment starts by
resetting the environment, which will provide initial observations and
extra information.

``` python
obs, extraInfo = env.reset()
```

Both, observation and extra information are provided as dictionaries
mapping from agent keys to the corresponding values. The agent keys are
the IDs that were used to register the agents in the ns3 simulation.

The current implementation does not enforce all agents to be present in
the observation and extra information dictionaries. This allows for a
flexible setup where agents do not need to act synchronously. The user
therefore has to check, whether observations from a particular agent
were actually received, before he can act on them. The easiest way to do
this is to simply iterate over the observation dictionary.

The following code snippet demonstrates how an action is randomly
sampled for each agent that shared its observation.

``` python
terminated = truncated = False
while not terminated and not truncated:
    action = {}
    for agent_id, agent_obs in obs.items():
        action[agent_id] = env.action_space[agent_id].sample()
    obs, reward, terminated, truncated, info = env.step(action)
    terminated = terminated["__all__"]
    truncated = truncated["__all__"]
```

Note how the action space (and equally the observation space) can be
inferred from the environment instance.

The step method takes a dictionary of actions as input. This dictionary
maps from agent_ids to actions and it is required that only actions for
agents that shared their observations are provided (but each of these
agents needs to receive an action). The method returns the new
observations, the rewards (also as a dictionary), a dictionary
indicating whether an agent reached a terminal state, a dictionary
indicating whether an agent was stopped due to a time limit, and the new
extra information.

The terminated and truncated dictionaries contain the special key
**\_\_all\_\_** that indicates whether all agents reached a terminal
state or were stopped due to a time limit. The user can use this
information to decide whether the environment should be reset or not.

All in all, this enables the user to build
arbitrarily complex training or evaluation loops.

In the following section, advanced topics will be discussed that might
be of interest to the user when working with the `Ns3MultiAgentEnv` but
are not necessary for basic usage.

### Advanced Usage

#### Random Seeding

Randomness is an often desired property in reinforcement learning
experiments. To ensure reproducibility, the user can set a seed for the
random number generator in the ns3 simulation. In ns3, seeds consist of
an overall seed and a run number.

The following code snippet demonstrates how the seed can be set in the
ns3 simulation:

``` cpp
int seed = 1;
int seedRunNumber = 1;
CommandLine cmd;
cmd.AddValue("seed", "The seed used for reproducibility", seed);
cmd.AddValue(
    "seedRunNumber",
    "Counts how often the environment has been reset (used for seeding)",
    seedRunNumber);
cmd.Parse(argc, argv);

RngSeedManager::SetSeed(seed);
RngSeedManager::SetRun(seedRunNumber);
```

In Python, the seed can be set in the `ns3Settings` dictionary:

``` python
ns3Settings: dict[str] = {"numAgents": 3, "seed": 1, "seedRunNumber": 1}
```

>[!NOTE]
>In order to achieve meaningful results it has to be ensured that the
>agents to not overfit during training. Therefore, a different seed
>should be used each time the environment is reset. This is done
>automatically when the argument `seedRunNumber` is provided to the
>`ns3Settings`. The run number is increased by one each time the
>environment is reset.

#### Registering the Environment

The method proposed in [Creating an Environment
Instance](#creating-an-environment-instance) is easy to use as long as
this environment shall exist in the same process as the driver Python
script. This is not the case for some distributed reinforcement learning
libraries like RLlib. The Gymnasium standard introduced a pattern to
deal with this issue. The user can register the environment via a string
identifier and a factory function that creates the environment instance.
The factory function is then called whenever the environment is
requested.

For Gymnasium, registering the environment would look like this:

``` python
import gymnasium
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv  # this import is necessary to register the environment

# specify the target name, the path to the ns3 root directory and the ns3 settings
# ...

gymnasium.envs.register(
    id="Multi-Agent-Env",
    entry_point="ns3ai_gym_env.envs:Ns3MultiAgentEnv",
    kwargs={
        "targetName": targetName,
        "ns3Path": ns3Path,
        "ns3Settings": ns3Settings,
    },
)
env = gymnasium.make("Multi-Agent-Env", disable_env_checker=True)
```

>[!NOTE]
>When registering an environment with Gymnasium, environment checking has
>to be disabled because Gymnasium assumes that all agents will have an
>initial observation after environment reset. In the model provided by
>this library, this is not the case.


In Ray RLlib the environment might be registered like this:

``` python
from ray.tune import register_env
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv  # this import is necessary to register the environment

# specify the target name, the path to the ns3 root directory and the ns3 settings
# ...
register_env(
    "Multi-Agent-Env",
    lambda _: Ns3MultiAgentEnv(
        targetName=targetName,
        ns3Path=ns3Path,
        ns3Settings=ns3Settings,
    ),
)
```


>[!NOTE]
>In case the user needs information that is inferred from an environment
>instance they can simply create a dummy instance, get the relevant
>information and immediately close the dummy instance.
>``` python
>dummy_env = Ns3MultiAgentEnv(targetName=targetName, ns3Path=ns3Path, ns3Settings=ns3Settings)
>obs_space = dummy_env.observation_space
>act_space = dummy_env.action_space
>dummy_env.close()
>```

#### Running Multiple Environments in Parallel

Executing multiple experiments in parallel often is an interesting use
case (e.g. for hyperparameter optimization). Because each environment
uses shared memory for communication with the ns3 simulation, it has to
be ensured that the environments do not interfere with each other. This
can be done by naming the memory segments for each newly created
environment instance. This can be done via the argument `trial_name`
that is passed in the ns3 settings.

Schematically, this might look similar to the following Python code
snippet:

``` python
trial = {"trial_name": 1}
ns3Settings: dict[str] = {"numAgents": 3, "seed": 1, "seedRunNumber": 1}

env1:Ns3MultiAgentEnv = Ns3MultiAgentEnv(targetName=targetName, ns3Path=ns3Path, ns3Settings=(ns3Settings | trial))

trial["trial_name"] += 1
env2:Ns3MultiAgentEnv = Ns3MultiAgentEnv(targetName=targetName, ns3Path=ns3Path, ns3Settings=(ns3Settings | trial))

# create many more environment instances
```

In practice, however, how the user sets the trial_name for each
environment has to fit the creation process of the environment
instances. The user must ensure that the trial_name is unique for each
environment instance.

Also, the trial name has to be set in the ns3 simulation. This can be
done by adding the following lines to the ns3 simulation:

``` cpp
std::string trial_name = "0";
CommandLine cmd;
cmd.AddValue("trial_name", "name of the trial", trial_name);
cmd.Parse(argc, argv);

OpenGymMultiAgentInterface::Get();
Ns3AiMsgInterface::Get()->SetNames("My Seg" + trial_name,
                                   "My Cpp to Python Msg" + trial_name,
                                   "My Python to Cpp Msg" + trial_name,
                                   "My Lockable" + trial_name);
```

>[!NOTE]
>"My Seg", "My Cpp to Python Msg", "My Python to Cpp Msg" and "My
>Lockable" are the default names of the memory segments that are used for
>communication between the ns3 simulation and the Python environment.

>[!NOTE]
>In case the setup is messed up and multiple environments use the same
>memory segments this will lead to strange behavior in the simulation. In
>case the segment names are not aligned between the ns3 simulation and
>the Python environment you will encounter the error message
>`boost::interprocess::bad_alloc`.

### Conclusion

The previous sections described how the `Ns3MultiAgentEnv` turns an ns3
simulation into a multi-agent environment that can be interacted with
according to the Gymnasium standard.

Check out the provided example scripts for even more information.
