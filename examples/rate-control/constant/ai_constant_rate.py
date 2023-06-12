#! /usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
# Copyright (c) 2021 Huazhong University of Science and Technology, Dian Group
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
# Author: Xun Deng <dorence@hust.edu.cn>
#         Hao Yin <haoyin@uw.edu>

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
    ns3Settings = {'raa': 'AiConstantRate', 'nWifi': 3, 'standard': '11ac', 'duration': 5}
    mempool_key = 1234 # memory pool key, arbitrary integer large than 1000
    mem_size = 4096 # memory pool size in bytes
    exp = Experiment(mempool_key, mem_size, 'rate-control', '../../', using_waf=False)
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
