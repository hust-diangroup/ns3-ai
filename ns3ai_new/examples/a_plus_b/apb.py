from ns3ai_apb import ApbNS3AIRL

rl = ApbNS3AIRL(4096, True)

while True:
    rl.local_act_clear()
    envs = rl.get_env()
    if rl.is_finished():
        break
    for env in envs:
        rl.local_act_push_back(env['env_a'] + env['env_b'])
    rl.set_act()

