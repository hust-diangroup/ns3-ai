from gymnasium.envs.registration import register

register(
    id="ns3ai_gym_env/Ns3-v0",
    entry_point="ns3ai_gym_env.envs:Ns3Env",
)
