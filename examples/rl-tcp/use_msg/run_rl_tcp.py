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

import os
import torch
import argparse
import numpy as np
import matplotlib.pyplot as plt
from agents import TcpNewRenoAgent, TcpRlAgent
import ns3ai_rltcp_msg_py as ns3ai_msg


def get_agent(socketUuid, useRl):
    agent = get_agent.tcpAgents.get(socketUuid)
    if agent is None:
        print("new agent, uuid = {}".format(socketUuid))
        if useRl:
            agent = TcpRlAgent()
        else:
            agent = TcpNewRenoAgent()
        get_agent.tcpAgents[socketUuid] = agent

    return agent


# initialize variable
get_agent.tcpAgents = {}

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--seed', type=int,
                        help='set seed for reproducibility')
    parser.add_argument('--show_log', action='store_true',
                        help='whether show observation and action')
    parser.add_argument('--result', action='store_true',
                        help='whether output figures')
    parser.add_argument('--result_dir', type=str,
                        default='./result', help='output figures path')
    parser.add_argument('--use_rl', action='store_true',
                        help='whether use rl algorithm')

    args = parser.parse_args()
    res_list = ['ssThresh_l', 'cWnd_l', 'segmentsAcked_l',
                'segmentSize_l', 'bytesInFlight_l']

    my_seed = 42
    if args.seed:
        my_seed = args.seed
    print("Using random seed {} for reproducibility.".format(my_seed))
    np.random.seed(my_seed)
    torch.manual_seed(my_seed)

    if args.result:
        for res in res_list:
            globals()[res] = []
        if args.result_dir:
            if not os.path.exists(args.result_dir):
                os.mkdir(args.result_dir)

    ns3ai = ns3ai_msg.Ns3AiMsgInterface(True, False, True, 4096, "My Seg", "My Cpp to Python Msg", "My Python to Cpp Msg", "My Lockable")
    print('Created message interface, waiting for C++ side to send initial environment...')

    while True:
        # receive observation from C++
        ns3ai.py_recv_begin()
        if ns3ai.py_get_finished():
            break
        ssThresh = ns3ai.m_single_cpp2py_msg.ssThresh
        cWnd = ns3ai.m_single_cpp2py_msg.cWnd
        segmentsAcked = ns3ai.m_single_cpp2py_msg.segmentsAcked
        segmentSize = ns3ai.m_single_cpp2py_msg.segmentSize
        bytesInFlight = ns3ai.m_single_cpp2py_msg.bytesInFlight
        socketId = ns3ai.m_single_cpp2py_msg.socketUid
        ns3ai.py_recv_end()

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
        ns3ai.py_send_begin()
        ns3ai.m_single_py2cpp_msg.new_cWnd = new_cWnd
        ns3ai.m_single_py2cpp_msg.new_ssThresh = new_ssThresh
        ns3ai.py_send_end()

        if args.show_log:
            print("Send act:", act)

    if args.result:
        for res in res_list:
            y = globals()[res]
            x = range(len(y))
            plt.clf()
            plt.plot(x, y, label=res[:-2], linewidth=1, color='r')
            plt.xlabel('Step Number')
            plt.title('Information of {}'.format(res[:-2]))
            plt.savefig('{}.png'.format(os.path.join(args.result_dir, res[:-2])))

    del ns3ai
