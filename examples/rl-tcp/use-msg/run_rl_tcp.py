# Copyright (c) 2020 Huazhong University of Science and Technology
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
#         Muyuan Shen <muyuan_shen@hust.edu.cn>


import os
import sys
import traceback
import torch
import argparse
import numpy as np
import matplotlib.pyplot as plt
from agents import TcpNewRenoAgent, TcpDeepQAgent, TcpQAgent
import ns3ai_rltcp_msg_py as py_binding
from ns3ai_utils import Experiment


def get_agent(socketUuid, useRl):
    agent = get_agent.tcpAgents.get(socketUuid)
    if agent is None:
        if useRl:
            if args.rl_algo == 'DeepQ':
                agent = TcpDeepQAgent()
                print("new Deep Q-learning agent, uuid = {}".format(socketUuid))
            else:
                agent = TcpQAgent()
                print("new Q-learning agent, uuid = {}".format(socketUuid))
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
parser.add_argument('--sim_seed', type=int,
                    help='set simulation run number')
parser.add_argument('--duration', type=float,
                    help='set simulation duration (seconds)')
parser.add_argument('--show_log', action='store_true',
                    help='whether show observation and action')
parser.add_argument('--result', action='store_true',
                    help='whether output figures')
parser.add_argument('--result_dir', type=str,
                    default='./rl_tcp_results', help='output figures path')
parser.add_argument('--use_rl', action='store_true',
                    help='whether use rl algorithm')
parser.add_argument('--rl_algo', type=str,
                    default='DeepQ', help='RL Algorithm, Q or DeepQ')

args = parser.parse_args()
my_seed = 42
if args.seed:
    my_seed = args.seed
print("Python side random seed {}".format(my_seed))
np.random.seed(my_seed)
torch.manual_seed(my_seed)

my_sim_seed = 0
if args.sim_seed:
    my_sim_seed = args.sim_seed

my_duration = 1000
if args.duration:
    my_duration = args.duration

if args.use_rl:
    if (args.rl_algo != 'Q') and (args.rl_algo != 'DeepQ'):
        print("Invalid RL Algorithm {}".format(args.rl_algo))
        exit(1)

res_list = ['ssThresh_l', 'cWnd_l', 'segmentsAcked_l',
            'segmentSize_l', 'bytesInFlight_l']
if args.result:
    for res in res_list:
        globals()[res] = []

stepIdx = 0

ns3Settings = {
    'transport_prot': 'TcpRlTimeBased',
    'duration': my_duration,
    'simSeed': my_sim_seed}
exp = Experiment("ns3ai_rltcp_msg", "../../../../../", py_binding, handleFinish=True)
msgInterface = exp.run(setting=ns3Settings, show_output=True)

try:
    while True:
        # receive observation from C++
        msgInterface.PyRecvBegin()
        if msgInterface.PyGetFinished():
            print("Simulation ended")
            break
        ssThresh = msgInterface.GetCpp2PyStruct().ssThresh
        cWnd = msgInterface.GetCpp2PyStruct().cWnd
        segmentsAcked = msgInterface.GetCpp2PyStruct().segmentsAcked
        segmentSize = msgInterface.GetCpp2PyStruct().segmentSize
        bytesInFlight = msgInterface.GetCpp2PyStruct().bytesInFlight
        socketId = msgInterface.GetCpp2PyStruct().socketUid
        msgInterface.PyRecvEnd()

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
        msgInterface.PySendBegin()
        msgInterface.GetPy2CppStruct().new_cWnd = new_cWnd
        msgInterface.GetPy2CppStruct().new_ssThresh = new_ssThresh
        msgInterface.PySendEnd()

        if args.show_log:
            print("Step:", stepIdx)
            stepIdx += 1
            print("Send act:", act)

except Exception as e:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    print("Exception occurred: {}".format(e))
    print("Traceback:")
    traceback.print_tb(exc_traceback)
    exit(1)

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
