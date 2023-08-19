import ns3ai_apb_py_stru as py_binding
from ns3ai_utils import Experiment

exp = Experiment("ns3ai_apb_msg_stru", "../../../../../", py_binding,
                 handleFinish=True)
msgInterface = exp.run(show_output=True)

try:
    while True:
        # receive from C++ side
        msgInterface.py_recv_begin()
        if msgInterface.py_get_finished():
            break
        # calculate the sum
        temp = msgInterface.m_single_cpp2py_msg.a + msgInterface.m_single_cpp2py_msg.b
        msgInterface.py_recv_end()

        # send to C++ side
        msgInterface.py_send_begin()
        msgInterface.m_single_py2cpp_msg.c = temp
        msgInterface.py_send_end()
except Exception as e:
    print("Exception occurred in experiment:")
    print(e)
finally:
    del exp
