'''
This script demonstrates how a reinforcement learning library (Ray RLlib) can be used to perform inference with a trained model in a ns3-simulation.
The script performs the following steps in order to evaluate the performance of a trained model:
1. Imports the necessary libraries and modules.
2. Restores the state of the training algorithm from a checkpoint (the environment needs to be registered in the same way as done in the training script).
3. Runs inference in multiple simulations via the policies from the resrored algorithm.
4. Closes the environment after the final simulation has ended.

Note: Some external libraries like Ray RLlib or Tensorflow are required to run this script.
'''

import argparse

from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv
from ray.rllib.algorithms.algorithm import Algorithm
from ray.rllib.utils.framework import try_import_tf
from ray.tune import register_env

# fix for the following issue: https://github.com/ray-project/ray/issues/14533
tf1, tf, tfv = try_import_tf()
tf1.enable_eager_execution()

parser = argparse.ArgumentParser()
parser.add_argument("--ns3Path", type=str, required=True, help="Path to the ns3 root directory.")
parser.add_argument("--checkpointPath", type=str, required=True, help="Path to the checkpoint to restore.")
parser.add_argument("--numAgents", type=int, default=3, help="Number of agents in the simulation.")
parser.add_argument("--numSimulations", type=int, default=10, help="Number of simulations to run.")
args = parser.parse_args()

targetName = "ns3ai_multi-agent"
ns3Settings: dict[str] = {"numAgents": args.numAgents, "seedRunNumber": 1}

register_env(
    "Multi-Agent-Env",
    lambda _: Ns3MultiAgentEnv(
        targetName=targetName,
        ns3Path=args.ns3Path,
        ns3Settings=ns3Settings,
    ),
)

restored_algo = Algorithm.from_checkpoint(
    args.checkpointPath, policies_to_train=lambda _: False
)
restored_algo.restore(args.checkpointPath)

env = Ns3MultiAgentEnv(targetName=targetName, ns3Path=args.ns3Path, ns3Settings=ns3Settings)

for simulation in range(args.numSimulations):
    simulation_reward = 0
    terminated = truncated = False
    obs, info = env.reset()
    step_count = 0
    while not terminated and not truncated:
        action = {}
        state = {}
        for agent_id, agent_obs in obs.items():
            policy_id = restored_algo.config.multi_agent()["policy_mapping_fn"](
                agent_id, None, None
            )
            action[agent_id] = restored_algo.compute_single_action(
                observation=agent_obs,
                policy_id=policy_id,
                explore=False,
                timestep=step_count,
            )
        obs, reward, terminated, truncated, info = env.step(action)
        simulation_reward += (
            list(reward.values())[0] if len(list(reward.values())) > 0 else 0
        )
        step_count += 1
        terminated = terminated["__all__"]
        truncated = truncated["__all__"]
    print(f"simulation {simulation} completed - mean reward: {simulation_reward / step_count}")

env.close()
