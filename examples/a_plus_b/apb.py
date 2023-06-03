import ns3ai_apb_py as apb

rl = apb.NS3AIRL(4096, True, "My Seg", "My Env", "My Act", "My Lockable")
myenvs = apb.PyEnvVector()
myacts = apb.PyActVector()
temp_act = apb.PyActStruct()

while True:
    myacts.clear()
    rl.get_env(myenvs)
    if rl.is_finished():
        break
    for i in myenvs:
        temp_act.c = i.a + i.b
        myacts.append(temp_act)
    rl.set_act(myacts)
