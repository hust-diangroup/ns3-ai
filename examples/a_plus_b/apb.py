import ns3ai_apb_py as apb

APB_SIZE = 3

rl = apb.Ns3AiRl(4096, True, True, "My Seg", "My Env", "My Act", "My Lockable")

assert len(rl.m_act) == 0
rl.m_act.resize(APB_SIZE)
assert len(rl.m_env) == 0
rl.m_env.resize(APB_SIZE)

while True:
    rl.get_env_begin()
    rl.set_act_begin()
    if rl.is_finished():
        break
    for i in range(len(rl.m_env)):
        rl.m_act[i].c = rl.m_env[i].a + rl.m_env[i].b
    rl.get_env_end()
    rl.set_act_end()

del rl
