'''
This script demonstrates how the Ns3MultiAgentEnv class can be used together with a specific ns3 simulation.

The script performs the following steps in order to evaluate the performance of random agents:
1. Imports the necessary libraries and modules.
2. Sets up logging configuration.
3. Defines the configuration for the ns3-simulation that shall be run.
5. Runs multiple episodes of the environment with actions sampled randomly from the action space.
6. Closes the environment after the final simulation has ended.

Note: This script assumes that the ns-3 simulator is already installed and the necessary dependencies are met.
'''

import argparse
import logging

from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv

logging.basicConfig(level=logging.INFO)  # verbosity can be reduced by changing this to warning
logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser()
parser.add_argument("--ns3Path", type=str, required=True, help="Path to the ns3 root directory.")
parser.add_argument("--numAgents", type=int, default=3, help="Number of agents in the simulation.")
parser.add_argument("--numSimulations", type=int, default=10, help="Number of simulations to run.")
args = parser.parse_args()

targetName = "ns3ai_multi-agent"
ns3Settings: dict[str] = {"numAgents": args.numAgents, "seedRunNumber": 1}

env = Ns3MultiAgentEnv(targetName=targetName, ns3Path=args.ns3Path, ns3Settings=ns3Settings)


for simulation in range(args.numSimulations):
    simulation_reward = 0
    terminated = truncated = False
    step_count = 0
    obs, info = env.reset()
    while not terminated and not truncated:
        action = {}
        for agent_id, agent_obs in obs.items():
            action[agent_id] = env.action_space[agent_id].sample()
        obs, reward, terminated, truncated, info = env.step(action)
        simulation_reward += list(reward.values())[0] if len(list(reward.values())) > 0 else 0
        step_count += 1
        terminated = terminated["__all__"]
        truncated = truncated["__all__"]
    print(f"simulation {simulation} completed - mean reward: {simulation_reward / step_count}")

env.close()
