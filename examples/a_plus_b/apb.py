import ns3ai_apb_py as apb

rl = apb.NS3AIRL(4096, True, "My Seg", "My Env", "My Act", "My Lockable")

temp_act = apb.PyActStruct()
temp_act.c = 1
assert len(rl.m_act) == 0
rl.m_act.resize(3, temp_act)

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

