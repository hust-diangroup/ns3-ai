# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
# Copyright (c) 2020 Huazhong University of Science and Technology, Dian Group
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
#         Hao Yin <haoyin@uw.edu>
# Modify: Muyuan Shen

import os
import torch
import argparse
import numpy as np
import matplotlib.pyplot as plt
from agents import TcpNewRenoAgent, TcpRlAgent
import ns3ai_rltcp_msg_py as py_binding
from ns3ai_utils import Experiment


def get_agent(socketUuid, useRl):
    agent = get_agent.tcpAgents.get(socketUuid)
    if agent is None:
        if useRl:
            agent = TcpRlAgent()
            print("new RL agent, uuid = {}".format(socketUuid))
        else:
            agent = TcpNewRenoAgent()
            print("new New Reno agent, uuid = {}".format(socketUuid))
        get_agent.tcpAgents[socketUuid] = agent

    return agent


# initialize variable
get_agent.tcpAgents = {}

parser = argparse.ArgumentParser()
parser.add_argument('--seed', type=int,
                    help='set seed for reproducibility')
parser.add_argument('--show_log', action='store_true',
                    help='whether show observation and action')
parser.add_argument('--result', action='store_true',
                    help='whether output figures')
parser.add_argument('--result_dir', type=str,
                    default='./rl_tcp_results', help='output figures path')
parser.add_argument('--use_rl', action='store_true',
                    help='whether use rl algorithm')

args = parser.parse_args()
my_seed = 42
if args.seed:
    my_seed = args.seed
print("Python side random seed {}".format(my_seed))
np.random.seed(my_seed)
torch.manual_seed(my_seed)

res_list = ['ssThresh_l', 'cWnd_l', 'segmentsAcked_l',
            'segmentSize_l', 'bytesInFlight_l']
if args.result:
    for res in res_list:
        globals()[res] = []

stepIdx = 0

ns3Settings = {
    'transport_prot': 'TcpRlTimeBased',
    'duration': 1000}
exp = Experiment("ns3ai_rltcp_msg", "../../../../../", py_binding, handleFinish=True)
msgInterface = exp.run(setting=ns3Settings, show_output=True)

try:
    while True:
        # receive observation from C++
        msgInterface.py_recv_begin()
        if msgInterface.py_get_finished():
            print("Simulation ended")
            break
        ssThresh = msgInterface.m_single_cpp2py_msg.ssThresh
        cWnd = msgInterface.m_single_cpp2py_msg.cWnd
        segmentsAcked = msgInterface.m_single_cpp2py_msg.segmentsAcked
        segmentSize = msgInterface.m_single_cpp2py_msg.segmentSize
        bytesInFlight = msgInterface.m_single_cpp2py_msg.bytesInFlight
        socketId = msgInterface.m_single_cpp2py_msg.socketUid
        msgInterface.py_recv_end()

        obs = [ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight]
        if args.show_log:
            print("Recv obs:", obs)

        if args.result:
            for res in res_list:
                globals()[res].append(globals()[res[:-2]])

        tcpAgent = get_agent(socketId, args.use_rl)
        act = tcpAgent.get_action(obs)
        new_cWnd = act[0]
        new_ssThresh = act[1]

        # send action to C++
        msgInterface.py_send_begin()
        msgInterface.m_single_py2cpp_msg.new_cWnd = new_cWnd
        msgInterface.m_single_py2cpp_msg.new_ssThresh = new_ssThresh
        msgInterface.py_send_end()

        if args.show_log:
            print("Step:", stepIdx)
            stepIdx += 1
            print("Send act:", act)

except Exception as e:
    print("Exception occurred in experiment:")
    print(e)

else:
    if args.result:
        if args.result_dir:
            if not os.path.exists(args.result_dir):
                os.mkdir(args.result_dir)
        for res in res_list:
            y = globals()[res]
            x = range(len(y))
            plt.clf()
            plt.plot(x, y, label=res[:-2], linewidth=1, color='r')
            plt.xlabel('Step Number')
            plt.title('Information of {}'.format(res[:-2]))
            plt.savefig('{}.png'.format(os.path.join(args.result_dir, res[:-2])))

finally:
    print("Finally exiting...")
    del exp
