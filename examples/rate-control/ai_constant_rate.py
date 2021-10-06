#! /usr/bin/env python
# -*- coding: utf-8 -*-

from ctypes import *
from py_interface import *

class AiConstantRateEnv(Structure):
    _pack_ = 1
    _fields_ = [
        ('transmitStreams', c_ubyte),
        ('supportedStreams', c_ubyte),
        ('msc', c_ubyte)
    ]


class AiConstantRateAct(Structure):
    _pack_ = 1
    _fields_ = [
        ('nss', c_ubyte),
        ('next_mcs', c_ubyte)
    ]


class AiConstantRateContainer:
    use_ns3ai = True

    def __init__(self, uid: int = 2333) -> None:
        self.rl = Ns3AIRL(uid, AiConstantRateEnv, AiConstantRateAct)
        # print('({})size: Env {} Act {}'.format(uid, sizeof(AiConstantRateEnv), sizeof(AiConstantRateAct)))
        pass

    def do(self, env: AiConstantRateEnv, act: AiConstantRateAct) -> AiConstantRateAct:
        # DoGetDataTxVector
        act.nss = min(env.transmitStreams, env.supportedStreams)
        if env.msc != 0xff:
            act.nss = 1 + env.msc // 8
        # set next_mcs as previous msc
        act.next_mcs = env.msc

        # uncomment to specify arbitrary MCS
        # act.next_mcs = 5
        return act


if __name__ == '__main__':
    ns3Settings = {'raa': 'AiConstantRate', 'nWifi': 3, 'standard': '11ac', 'duration': 1}
    mempool_key = 1234 # memory pool key, arbitrary integer large than 1000
    mem_size = 4096 # memory pool size in bytes
    exp = Experiment(mempool_key, mem_size, 'rate-control', '../../')
    exp.reset()

    memblock_key = 2333 # memory block key in the memory pool, arbitrary integer, and need to keep the same in the ns-3 script
    c = AiConstantRateContainer(memblock_key)

    pro = exp.run(setting=ns3Settings, show_output=True)
    print("run rate-control", ns3Settings)
    while not c.rl.isFinish():
        with c.rl as data:
            if data == None:
                break
            data.act = c.do(data.env, data.act)
            pass
    
    pro.wait()
    del exp
