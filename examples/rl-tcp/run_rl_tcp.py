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

from py_interface import *
from ctypes import *
import os
import torch
import argparse
import numpy as np
import torch.nn as nn
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument('--result', action='store_true',
                    help='whether output figures')
parser.add_argument('--output_dir', type=str,
                    default='./result', help='output figures path')
parser.add_argument('--use_rl', action='store_true',
                    help='whether use rl algorithm')


class TcpRlEnv(Structure):
    _pack_ = 1
    _fields_ = [
        ('nodeId', c_uint32),
        ('socketUid', c_uint32),
        ('envType', c_uint8),
        ('simTime_us', c_int64),
        ('ssThresh', c_uint32),
        ('cWnd', c_uint32),
        ('segmentSize', c_uint32),
        ('segmentsAcked', c_uint32),
        ('bytesInFlight', c_uint32),
    ]


class TcpRlAct(Structure):
    _pack_ = 1
    _fields_ = [
        ('new_ssThresh', c_uint32),
        ('new_cWnd', c_uint32)
    ]


class net(nn.Module):
    def __init__(self):
        super(net, self).__init__()
        self.layers = nn.Sequential(
            nn.Linear(5, 20),
            nn.ReLU(),
            nn.Linear(20, 20),
            nn.ReLU(),
            nn.Linear(20, 4),
        )

    def forward(self, x):
        return self.layers(x)


class DQN(object):
    def __init__(self):
        self.eval_net = net()
        self.target_net = net()
        self.learn_step = 0
        self.batchsize = 32
        self.observer_shape = 5
        self.target_replace = 100
        self.memory_counter = 0
        self.memory_capacity = 2000
        self.memory = np.zeros((2000, 2*5+2))    # s, a, r, s'
        self.optimizer = torch.optim.Adam(
            self.eval_net.parameters(), lr=0.0001)
        self.loss_func = nn.MSELoss()

    def choose_action(self, x):
        x = torch.Tensor(x)
        if np.random.uniform() > 0.99 ** self.memory_counter:    # choose best
            action = self.eval_net.forward(x)
            action = torch.argmax(action, 0).numpy()
        else:    # explore
            action = np.random.randint(0, 4)
        return action

    def store_transition(self, s, a, r, s_):
        index = self.memory_counter % self.memory_capacity
        self.memory[index, :] = np.hstack((s, [a, r], s_))
        self.memory_counter += 1

    def learn(self, ):
        self.learn_step += 1
        if self.learn_step % self.target_replace == 0:
            self.target_net.load_state_dict(self.eval_net.state_dict())
        sample_list = np.random.choice(self.memory_capacity, self.batchsize)
        # choose a mini batch
        sample = self.memory[sample_list, :]
        s = torch.Tensor(sample[:, :self.observer_shape])
        a = torch.LongTensor(
            sample[:, self.observer_shape:self.observer_shape+1])
        r = torch.Tensor(
            sample[:, self.observer_shape+1:self.observer_shape+2])
        s_ = torch.Tensor(sample[:, self.observer_shape+2:])
        q_eval = self.eval_net(s).gather(1, a)
        q_next = self.target_net(s_).detach()
        q_target = r + 0.8 * q_next.max(1, True)[0].data

        loss = self.loss_func(q_eval, q_target)

        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()


Init(1234, 4096)
var = Ns3AIRL(1234, TcpRlEnv, TcpRlAct)
res_list = ['ssThresh_l', 'cWnd_l', 'segmentsAcked_l',
            'segmentSize_l', 'bytesInFlight_l']
args = parser.parse_args()

if args.result:
    for res in res_list:
        globals()[res] = []
    if args.output_dir:
        if not os.path.exists(args.output_dir):
            os.mkdir(args.output_dir)

if args.use_rl:
    dqn = DQN()
exp = Experiment(1234, 4096, 'rl-tcp', '../../')
exp.run(show_output=1)
try:
    while not var.isFinish():
        with var as data:
            if not data:
                break
    #         print(var.GetVersion())
            ssThresh = data.env.ssThresh
            cWnd = data.env.cWnd
            segmentsAcked = data.env.segmentsAcked
            segmentSize = data.env.segmentSize
            bytesInFlight = data.env.bytesInFlight
    #         print(ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight)

            if args.result:
                for res in res_list:
                    globals()[res].append(globals()[res[:-2]])
                    #print(globals()[res][-1])

            if not args.use_rl:
                new_cWnd = 1
                new_ssThresh = 1
                # IncreaseWindow
                if (cWnd < ssThresh):
                    # slow start
                    if (segmentsAcked >= 1):
                        new_cWnd = cWnd + segmentSize
                if (cWnd >= ssThresh):
                    # congestion avoidance
                    if (segmentsAcked > 0):
                        adder = 1.0 * (segmentSize * segmentSize) / cWnd
                        adder = int(max(1.0, adder))
                        new_cWnd = cWnd + adder
                # GetSsThresh
                new_ssThresh = int(max(2 * segmentSize, bytesInFlight / 2))
                data.act.new_cWnd = new_cWnd
                data.act.new_ssThresh = new_ssThresh
            else:
                s = [ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight]
                a = dqn.choose_action(s)
                if a & 1:
                    new_cWnd = cWnd + segmentSize
                else:
                    if(cWnd > 0):
                        new_cWnd = cWnd + int(max(1, (segmentSize * segmentSize) / cWnd))
                if a < 3:
                    new_ssThresh = 2 * segmentSize
                else:
                    new_ssThresh = int(bytesInFlight / 2)
                data.act.new_cWnd = new_cWnd
                data.act.new_ssThresh = new_ssThresh

                ssThresh = data.env.ssThresh
                cWnd = data.env.cWnd
                segmentsAcked = data.env.segmentsAcked
                segmentSize = data.env.segmentSize
                bytesInFlight = data.env.bytesInFlight

                # modify the reward
                r = segmentsAcked - bytesInFlight - cWnd
                s_ = [ssThresh, cWnd, segmentsAcked,
                      segmentSize, bytesInFlight]

                dqn.store_transition(s, a, r, s_)

                if dqn.memory_counter > dqn.memory_capacity:
                    dqn.learn()
except KeyboardInterrupt:
    exp.kill()
    del exp

if args.result:
    for res in res_list:
        y = globals()[res]
        x = range(len(y))
        plt.clf()
        plt.plot(x, y, label=res[:-2], linewidth=1, color='r')
        plt.xlabel('Step Number')
        plt.title('Information of {}'.format(res[:-2]))
        plt.savefig('{}.png'.format(os.path.join(args.output_dir, res[:-2])))
