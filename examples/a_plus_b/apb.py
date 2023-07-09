import ns3ai_apb_py as apb

APB_SIZE = 3

rl = apb.Ns3AiMsgInterface(4096, True, True, "My Seg", "My Env", "My Act", "My Lockable")

assert len(rl.m_py2cpp_msg) == 0
rl.m_py2cpp_msg.resize(APB_SIZE)
assert len(rl.m_cpp2py_msg) == 0
rl.m_cpp2py_msg.resize(APB_SIZE)

while True:
    rl.py_recv_begin()
    rl.py_send_begin()
    if rl.py_check_finished():
        break
    for i in range(len(rl.m_cpp2py_msg)):
        rl.m_py2cpp_msg[i].c = rl.m_cpp2py_msg[i].a + rl.m_cpp2py_msg[i].b
    rl.py_recv_end()
    rl.py_send_end()

del rl
