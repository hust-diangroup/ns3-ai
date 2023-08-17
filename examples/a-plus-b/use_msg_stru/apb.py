import ns3ai_apb_py_not_vec as apb

rl = apb.Ns3AiMsgInterface(True, False, True, 4096, "My Seg", "My Cpp to Python Msg", "My Python to Cpp Msg", "My Lockable")

print('Created message interface, waiting for C++ side to send initial environment...')

while True:
    rl.py_recv_begin()
    if rl.py_get_finished():
        break
    temp = rl.m_single_cpp2py_msg.a + rl.m_single_cpp2py_msg.b
    rl.py_recv_end()

    rl.py_send_begin()
    rl.m_single_py2cpp_msg.c = temp
    rl.py_send_end()

del rl
