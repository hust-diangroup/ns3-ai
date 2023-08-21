import ns3ai_apb_py_vec as py_binding
from ns3ai_utils import Experiment

APB_SIZE = 3

exp = Experiment("ns3ai_apb_msg_vec", "../../../../../", py_binding,
                 handleFinish=True, useVector=True, vectorSize=APB_SIZE)
msgInterface = exp.run(show_output=True)

try:
    while True:
        # receive from C++ side
        msgInterface.PyRecvBegin()
        if msgInterface.PyGetFinished():
            break

        # send to C++ side
        msgInterface.PySendBegin()
        for i in range(len(msgInterface.GetCpp2PyVector())):
            # calculate the sums
            msgInterface.GetPy2CppVector()[i].c = msgInterface.GetCpp2PyVector()[i].a + msgInterface.GetCpp2PyVector()[i].b
        msgInterface.PyRecvEnd()
        msgInterface.PySendEnd()

except Exception as e:
    print("Exception occurred in experiment:")
    print(e)

else:
    pass

finally:
    print("Finally exiting...")
    del exp
