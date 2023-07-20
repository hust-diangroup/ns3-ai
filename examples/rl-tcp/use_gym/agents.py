import torch
import numpy as np
import torch.nn as nn


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


class TcpRlAgent:

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
