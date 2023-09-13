# Copyright (c) 2023 Huazhong University of Science and Technology
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Muyuan Shen <muyuan_shen@hust.edu.cn>


import ns3ai_gym_env
import gymnasium as gym
import sys
import traceback

APB_SIZE = 3


class ApbAgent:

    def __init__(self):
        pass

    def get_action(self, obs, reward, done, info):

        a = obs[0]
        b = obs[1]
        act = a + b

        return [act]

env = gym.make("ns3ai_gym_env/Ns3-v0", targetName="ns3ai_apb_gym", ns3Path="../../../../../")
ob_space = env.observation_space
ac_space = env.action_space
print("Observation space: ", ob_space, ob_space.dtype)
print("Action space: ", ac_space, ac_space.dtype)

try:
    obs, info = env.reset()
    # print("---obs: ", obs)
    reward = 0
    done = False

    agent = ApbAgent()

    while True:

        action = agent.get_action(obs, reward, info, done)
        # print("---action: ", action)

        obs, reward, done, _, info = env.step(action)
        # print("---obs, reward, done, info: ", obs, reward, done, info)

        if done:
            break

except Exception as e:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    print("Exception occurred: {}".format(e))
    print("Traceback:")
    traceback.print_tb(exc_traceback)
    exit(1)

else:
    pass

finally:
    print("Finally exiting...")
    env.close()
