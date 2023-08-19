import ns3ai_apb_py_vec as py_binding
from ns3ai_utils import Experiment

APB_SIZE = 3

exp = Experiment("ns3ai_apb_msg_vec", "../../../../../", py_binding,
                 handleFinish=True, useVector=True, vectorSize=APB_SIZE)
msgInterface = exp.run(show_output=True)

try:
    while True:
        # receive from C++ side
        msgInterface.py_recv_begin()
        if msgInterface.py_get_finished():
            break

        # send to C++ side
        msgInterface.py_send_begin()
        for i in range(len(msgInterface.m_cpp2py_msg)):
            # calculate the sums
            msgInterface.m_py2cpp_msg[i].c = msgInterface.m_cpp2py_msg[i].a + msgInterface.m_cpp2py_msg[i].b
        msgInterface.py_recv_end()
        msgInterface.py_send_end()
except Exception as e:
    print("Exception occurred in experiment:")
    print(e)
finally:
    del exp
