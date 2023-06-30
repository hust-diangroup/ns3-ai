from gymnasium.envs.registration import register

register(
    id="apb_gym/APlusB-v0",
    entry_point="apb_gym.envs:APlusBEnv",
)
