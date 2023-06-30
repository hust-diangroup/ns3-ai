import apb_gym
import gymnasium as gym

env = gym.make("apb_gym/APlusB-v0")

_, info = env.reset(seed=42)

# print(observation, info)

while True:
    observation, reward, terminated, _, info = env.step(info["sum"])
    # print(observation, info)
    if terminated:
        break

env.close()
