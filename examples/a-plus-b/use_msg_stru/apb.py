import ns3ai_apb_py_stru as py_binding
from ns3ai_utils import Experiment
import sys
import traceback

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
    exc_type, exc_value, exc_traceback = sys.exc_info()
    print("Exception occurred: {}".format(e))
    print("Traceback:")
    traceback.print_tb(exc_traceback)
    exit(1)

else:
    pass

finally:
    print("Finally exiting...")
    del exp
