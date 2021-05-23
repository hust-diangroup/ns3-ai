# An example for the ns3-ai model to illustrate the data exchange 
# between python-based AI frameworks and ns-3. 
# 
# In this example, we have two variable a and b in ns-3, 
# and then put them into the shared memory using python to calculate
# 
#       c = a + b
# 
# Finally, we put back c to the ns-3. 

from py_interface import *
from ctypes import *
import sys
import ns3_util
import time


# The environment (in this example, contain 'a' and 'b')
# shared between ns-3 and python with the same shared memory
# using the ns3-ai model.
class Env(Structure):
    _pack_ = 1
    _fields_ = [
        ('a', c_int),
        ('b', c_int)
    ]

# The result (in this example, contain 'c') calculated by python 
# and put back to ns-3 with the shared memory.
class Act(Structure):
    _pack_ = 1
    _fields_ = [
        ('c', c_int)
    ]


ns3Settings = {'a': '20', 'b': '30'}
mempool_key = 1234                                          # memory pool key, arbitrary integer large than 1000
mem_size = 4096                                             # memory pool size in bytes
memblock_key = 2333                                         # memory block key, need to keep the same in the ns-3 script
exp = Experiment(mempool_key, mem_size, 'multi-run', '../../')      # Set up the ns-3 environment
for i in range(2):
    exp.reset()                                             # Reset the environment
    rl = Ns3AIRL(memblock_key, Env, Act)                    # Link the shared memory block with ns-3 script
    pro = exp.run(setting=ns3Settings, show_output=True)    # Set and run the ns-3 script (sim.cc)
    while not rl.isFinish():
        with rl as data:
            if data == None:
                break

            # AI algorithms here and put the data back to the action
            data.act.c = data.env.a+data.env.b
    pro.wait()                                              # Wait the ns-3 to stop
del exp