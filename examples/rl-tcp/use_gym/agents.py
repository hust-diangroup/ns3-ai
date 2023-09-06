# Copyright (c) 2023 Huazhong University of Science and Technology
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
# Author: Muyuan Shen <muyuan_shen@hust.edu.cn>


import torch
import numpy as np
import torch.nn as nn
import random


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
        self.memory = np.zeros((2000, 2 * 5 + 2))  # s, a, r, s'
        self.optimizer = torch.optim.Adam(
            self.eval_net.parameters(), lr=0.0001)
        self.loss_func = nn.MSELoss()

    def choose_action(self, x):
        x = torch.Tensor(x)
        if np.random.uniform() > 0.99 ** self.memory_counter:  # choose best
            action = self.eval_net.forward(x)
            action = torch.argmax(action, 0).numpy()
        else:  # explore
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
            sample[:, self.observer_shape:self.observer_shape + 1])
        r = torch.Tensor(
            sample[:, self.observer_shape + 1:self.observer_shape + 2])
        s_ = torch.Tensor(sample[:, self.observer_shape + 2:])
        q_eval = self.eval_net(s).gather(1, a)
        q_next = self.target_net(s_).detach()
        q_target = r + 0.8 * q_next.max(1, True)[0].data

        loss = self.loss_func(q_eval, q_target)

        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()


class TcpNewRenoAgent:

    def __init__(self):
        self.new_cWnd = 0
        self.new_ssThresh = 0
        pass

    def get_action(self, obs, reward, done, info):
        # current ssThreshold
        ssThresh = obs[4]
        # current contention window size
        cWnd = obs[5]
        # segment size
        segmentSize = obs[6]
        # number of acked segments
        segmentsAcked = obs[9]
        # estimated bytes in flight
        bytesInFlight = obs[7]

        self.new_cWnd = 1
        if cWnd < ssThresh:
            # slow start
            if segmentsAcked >= 1:
                self.new_cWnd = cWnd + segmentSize
        if cWnd >= ssThresh:
            # congestion avoidance
            if segmentsAcked > 0:
                adder = 1.0 * (segmentSize * segmentSize) / cWnd
                adder = int(max(1.0, adder))
                self.new_cWnd = cWnd + adder

        self.new_ssThresh = int(max(2 * segmentSize, bytesInFlight / 2))
        return [self.new_ssThresh, self.new_cWnd]


class TcpDeepQAgent:

    def __init__(self):
        self.dqn = DQN()
        self.new_cWnd = None
        self.new_ssThresh = None
        self.s = None
        self.a = None
        self.r = None
        self.s_ = None  # next state

    def get_action(self, obs, reward, done, info):
        # current ssThreshold
        ssThresh = obs[4]
        # current contention window size
        cWnd = obs[5]
        # segment size
        segmentSize = obs[6]
        # number of acked segments
        segmentsAcked = obs[9]
        # estimated bytes in flight
        bytesInFlight = obs[7]

        # update DQN
        self.s = self.s_
        self.s_ = [ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight]
        if self.s is not None:  # not first time
            self.r = segmentsAcked - bytesInFlight - cWnd
            self.dqn.store_transition(self.s, self.a, self.r, self.s_)
            if self.dqn.memory_counter > self.dqn.memory_capacity:
                self.dqn.learn()

        # choose action
        self.a = self.dqn.choose_action(self.s_)
        if self.a & 1:
            self.new_cWnd = cWnd + segmentSize
        else:
            if cWnd > 0:
                self.new_cWnd = cWnd + int(max(1, (segmentSize * segmentSize) / cWnd))
        if self.a < 3:
            self.new_ssThresh = 2 * segmentSize
        else:
            self.new_ssThresh = int(bytesInFlight / 2)

        return [self.new_ssThresh, self.new_cWnd]


class TcpQAgent:

    def discretize(self, metric, minval, maxval):
        metric = max(metric, minval)
        metric = min(metric, maxval)
        return int((metric - minval) * (self.discrete_level - 1) / (maxval - minval))

    def __init__(self):
        self.update_times = 0
        self.learning_rate = None
        self.discount_rate = 0.5
        self.discrete_level = 15
        self.epsilon = 0.1  # exploration rate
        self.state_size = 3
        self.action_size = 1
        self.action_num = 4
        self.actions = np.arange(self.action_num, dtype=int)
        self.q_table = np.zeros((*((self.discrete_level, ) * self.state_size), self.action_num))
        # print(self.q_table.shape)
        self.new_cWnd = None
        self.new_ssThresh = None
        self.s = None
        self.a = np.zeros(self.action_size, dtype=int)
        self.r = None
        self.s_ = None  # next state

    def get_action(self, obs, reward, done, info):
        # current ssThreshold
        # ssThresh = obs[4]
        # current contention window size
        cWnd = obs[5]
        # segment size
        segmentSize = obs[6]
        # number of acked segments
        segmentsAcked = obs[9]
        # estimated bytes in flight
        bytesInFlight = obs[7]

        cWnd_d = self.discretize(cWnd, 0., 50000.)
        segmentsAcked_d = self.discretize(segmentsAcked, 0., 64.)
        bytesInFlight_d = self.discretize(bytesInFlight, 0., 1000000.)

        self.s = self.s_
        self.s_ = [cWnd_d, segmentsAcked_d, bytesInFlight_d]
        if self.s:  # not first time
            # update Q-table
            self.learning_rate = 0.3 * (0.995 ** (self.update_times // 10))
            self.r = segmentsAcked - bytesInFlight - cWnd
            self.q_table[tuple(self.s)][tuple(self.a)] = (
                    (1 - self.learning_rate) * self.q_table[tuple(self.s)][tuple(self.a)] +
                    self.learning_rate * (self.r + self.discount_rate * np.max(self.q_table[tuple(self.s_)]))
            )
            self.update_times += 1

        # epsilon-greedy
        if random.uniform(0, 1) < 0.1:
            self.a[0] = np.random.choice(self.actions)
        else:
            self.a[0] = np.argmax(self.q_table[tuple(self.s_)])

        # map action to cwnd and ssthresh
        if self.a[0] & 1:
            self.new_cWnd = cWnd + segmentSize
        else:
            if cWnd > 0:
                self.new_cWnd = cWnd + int(max(1, (segmentSize * segmentSize) / cWnd))
        if self.a[0] < 3:
            self.new_ssThresh = 2 * segmentSize
        else:
            self.new_ssThresh = int(bytesInFlight / 2)

        return [self.new_ssThresh, self.new_cWnd]
