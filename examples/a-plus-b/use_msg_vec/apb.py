# Copyright (c) 2023 Huazhong University of Science and Technology
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Muyuan Shen <muyuan_shen@hust.edu.cn>


import ns3ai_apb_py_vec as py_binding
from ns3ai_utils import Experiment
import sys
import traceback

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
