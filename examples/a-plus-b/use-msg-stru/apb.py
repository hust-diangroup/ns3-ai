# Copyright (c) 2019-2023 Huazhong University of Science and Technology
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
# Author: Pengyu Liu <eic_lpy@hust.edu.cn>
#         Xiaojun Guo <guoxj@hust.edu.cn>
#         Hao Yin <haoyin@uw.edu>
#         Muyuan Shen <muyuan_shen@hust.edu.cn>


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
