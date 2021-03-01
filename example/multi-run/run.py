from py_interface import *
from ctypes import *
import sys
import ns3_util
import time

class Env(Structure):
    _pack_ = 1
    _fields_ = [
        ('a', c_int),
        ('b', c_int)
    ]


class Act(Structure):
    _pack_ = 1
    _fields_ = [
        ('c', c_int)
    ]
exp = Experiment(1234, 4096, 'multi-run', '../../')
for i in range(2):
    exp.reset()
    rl = Ns3AIRL(2333, Env, Act)
    pro = exp.run(show_output=True)
    while not rl.isFinish():
        with rl as data:
            if data == None:
                break
            data.act.c = data.env.a+data.env.b
    pro.wait()
del exp
