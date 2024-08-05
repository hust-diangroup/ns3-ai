"""
This example demonstrates how a reinforcement learning library (Ray RLlib) can be used to train multiple agents in a ns3-simulation with DQN.
The script performs the following steps in order to train the agents:
1. Imports the necessary libraries and modules.
2. Registers the environment using ray tune.
3. Configures the training algorithm.
4. Trains the agents for multiple iterations and prints relevant metrics.

These are the most essential steps to train in any multi-agent environment using Ray RLlib. For advanced usage like hyperparameter tuning please refer to the Ray RLlib documentation and the advanced usage section of this modules documentation.

Note: Some external libraries like Ray RLlib or Tensorflow are required to run this script.
"""

import argparse
from pprint import pprint as pp

from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv
from ray.rllib.algorithms.dqn import DQNConfig
from ray.rllib.policy.policy import PolicySpec
from ray.tune import register_env

parser = argparse.ArgumentParser()
parser.add_argument("--ns3Path", type=str, required=True, help="Path to the ns3 root directory.")
parser.add_argument("--checkpointPath", type=str, required=True, help="Path to the checkpoint to restore.")
parser.add_argument("--numAgents", type=int, default=3, help="Number of agents in the simulation.")
parser.add_argument("--numIterations", type=int, default=50, help="Number of training iterations to run.")
args = parser.parse_args()

targetName = "ns3ai_multi-agent"
ns3Settings: dict[str] = {"numAgents": args.numAgents, "seedRunNumber": 1}

env = Ns3MultiAgentEnv(targetName=targetName, ns3Path=args.ns3Path, ns3Settings=ns3Settings)
env_obs_space = env.observation_space
env_act_space = env.action_space
env.close()

register_env(
    "Multi-Agent-Env",
    lambda _: Ns3MultiAgentEnv(
        targetName=targetName,
        ns3Path=args.ns3Path,
        ns3Settings=ns3Settings,
    ),
)

replay_config = {
    "type": "MultiAgentPrioritizedReplayBuffer",
    "capacity": 60000,
    "prioritized_replay_alpha": 0.5,
    "prioritized_replay_beta": 0.5,
    "prioritized_replay_eps": 3e-6,
}

config = (
    DQNConfig()
    .training(train_batch_size=1024, replay_buffer_config=replay_config)
    .resources(num_gpus=0)
    .rollouts(num_rollout_workers=1, batch_mode="complete_episodes")
    .environment("Multi-Agent-Env")
    .framework("tf2")
    .multi_agent(
        policies={
            agent_id: PolicySpec(
                observation_space=env_obs_space[agent_id],
                action_space=env_act_space[agent_id],
            )
            for agent_id in env_obs_space.keys()
        },
        policy_mapping_fn=lambda agent_id, episode, worker, **kwargs: agent_id,
    )
    .debugging(log_level="ERROR")
)

algo = config.build()

metrics_to_print = [
    "episode_reward_mean",
    "episode_reward_max",
    "episode_reward_min",
    "counters",
]

for i in range(args.numIterations):
    print(f"New training iteration {i} started:")
    result = algo.train()
    pp({k: v for k, v in result.items() if k in metrics_to_print})

# checkpointing
save_result = algo.save(args.checkpointPath)
path_to_checkpoint = save_result.checkpoint.path
print(
    "An Algorithm checkpoint has been created inside directory: "
    f"'{path_to_checkpoint}'."
)

# final cleanup to free resources
algo.cleanup()
