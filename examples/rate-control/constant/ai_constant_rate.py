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

# import sys
# sys.path.append("../../../utils")
# import ns3_util

import ns3ai_ratecontrol_constant_py as cr

class AiConstantRateContainer:
    use_ns3ai = True

    def __init__(self) -> None:
        self.rl = cr.Ns3AiMsgInterface(True, False, 4096, "My Seg", "My Env", "My Act", "My Lockable")
        # print('({})size: Env {} Act {}'.format(uid, sizeof(AiConstantRateEnv), sizeof(AiConstantRateAct)))
        pass

    def __del__(self):
        del self.rl

    def do(self, env: cr.PyEnvStruct, act: cr.PyActStruct):
        # DoGetDataTxVector
        act.nss = min(env.transmitStreams, env.supportedStreams)
        if env.msc != 0xff:
            act.nss = 1 + env.msc // 8
        # set next_mcs as previous msc
        act.next_mcs = env.msc

        # uncomment to specify arbitrary MCS
        # act.next_mcs = 5


if __name__ == '__main__':
    # ns3Settings = {'raa': 'AiConstantRate', 'nWifi': 3, 'standard': '11ac', 'duration': 5}
    # mempool_key = 1234 # memory pool key, arbitrary integer large than 1000
    # mem_size = 4096 # memory pool size in bytes
    # exp = Experiment(mempool_key, mem_size, 'rate-control', '../../', using_waf=False)
    # exp.reset()

    c = AiConstantRateContainer()

    # pro = exp.run(setting=ns3Settings, show_output=True)
    # print("run rate-control", ns3Settings)
    while True:
        c.rl.py_recv_begin()
        c.rl.py_send_begin()
        if c.rl.py_check_finished():
            break
        c.do(c.rl.m_single_cpp2py_msg, c.rl.m_single_py2cpp_msg)
        c.rl.py_recv_end()
        c.rl.py_send_end()

    del c
