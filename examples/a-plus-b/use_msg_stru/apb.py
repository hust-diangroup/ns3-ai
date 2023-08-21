import ns3ai_apb_py_stru as py_binding
from ns3ai_utils import Experiment

exp = Experiment("ns3ai_apb_msg_stru", "../../../../../", py_binding,
                 handleFinish=True)
msgInterface = exp.run(show_output=True)

try:
    while True:
        # receive from C++ side
        msgInterface.PyRecvBegin()
        if msgInterface.PyGetFinished():
            break
        # calculate the sum
        temp = msgInterface.GetCpp2PyStruct().a + msgInterface.GetCpp2PyStruct().b
        msgInterface.PyRecvEnd()

        # send to C++ side
        msgInterface.PySendBegin()
        msgInterface.GetPy2CppStruct().c = temp
        msgInterface.PySendEnd()

except Exception as e:
    print("Exception occurred in experiment:")
    print(e)

else:
    pass

finally:
    print("Finally exiting...")
    del exp
