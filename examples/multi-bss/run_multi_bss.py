import random
import os
import numpy as np
import matplotlib.pyplot as plt
import pylab as pl
import matplotlib
from mpl_toolkits.axes_grid1 import host_subplot
from collections import namedtuple, deque
from itertools import count
import math
import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.autograd import Variable
import torch.optim as optim
import subprocess
import ns3ai_multibss_py as ns3ai


device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
is_ipython = 'inline' in matplotlib.get_backend()
if is_ipython:
    from IPython import display

plt.ion()


Transition = namedtuple('Transition',
                        ('state', 'action', 'next_state', 'reward'))


class ReplayMemory(object):

    def __init__(self, capacity):
        self.memory = deque([], maxlen=capacity)

    def push(self, *args):
        """Save a transition"""
        self.memory.append(Transition(*args))

    def sample(self, batch_size):
        return random.sample(self.memory, batch_size)

    def __len__(self):
        return len(self.memory)

class DQN(nn.Module):

    def __init__(self, n_observations, n_actions):
        super(DQN, self).__init__()
        self.layer1 = nn.Linear(n_observations, 128)
        self.layer2 = nn.Linear(128, 128)
        self.layer3 = nn.Linear(128, n_actions)

    # Called with either one element to determine next action, or a batch
    # during optimization. Returns tensor([[left0exp,right0exp]...]).
    def forward(self, x):
        x = F.relu(self.layer1(x))
        x = F.relu(self.layer2(x))
        return self.layer3(x)


# ns3Settings = {
#     'pktSize': 1500,
#     'duration':200,
#     'gi': 800,
#     'channelWidth': 20,
#     'rng': 2,
#     'apNodes': 4,
#     'networkSize': 4,
#     'ring': 0,
#     'maxMpdus': 5,
#     'autoMCS': True,
#     'prop': 'tgax',
#     'app': 'setup-done',
#     'pktInterval': 5000,
#     'boxsize': 25,
#     'drl': True,
# }
# mempool_key = 1234                                          # memory pool key, arbitrary integer large than 1000
# mem_size = 4096                                             # memory pool size in bytes
# memblock_key = 2333                                         # memory block key, need to keep the same in the ns-3 script
# using_waf = False

# n_ap = int(ns3Settings['apNodes'])
# n_sta = int(ns3Settings['networkSize'])
n_ap = 4
n_sta = 4
n_total = n_ap * (n_sta + 1)
state = np.zeros((n_sta+1, n_total+1))
rewards = []
overall_rewards = []
# print(state.shape)

BATCH_SIZE = 32
GAMMA = 0.99
EPS_START = 0.9
EPS_END = 0.05
EPS_DECAY = 50
TAU = 0.005
LR = 1e-4

# Get number of actions from gym action space
n_actions = -62 - (-82) + 1
n_observations = (n_sta + 1) * (n_total + 1)

policy_net = DQN(n_observations, n_actions).to(device)
target_net = DQN(n_observations, n_actions).to(device)
target_net.load_state_dict(policy_net.state_dict())

optimizer = optim.AdamW(policy_net.parameters(), lr=LR, amsgrad=True)
memory = ReplayMemory(200)


steps_done = 0


def select_action(state):
    global steps_done
    sample = random.random()
    eps_threshold = EPS_END + (EPS_START - EPS_END) * \
                    math.exp(-1. * steps_done / EPS_DECAY)
    steps_done += 1
    if sample > eps_threshold:
        with torch.no_grad():
            # t.max(1) will return the largest column value of each row.
            # second column on max result is index of where max element was
            # found, so we pick action with the larger expected reward.
            return policy_net(state).max(1)[1].view(1, 1)
    else:
        return torch.tensor([[np.random.randint(0, n_actions)]], device=device, dtype=torch.long)


episode_durations = []


def plot_durations(show_result=False):
    plt.figure(1)
    durations_t = torch.tensor(episode_durations, dtype=torch.float)
    if show_result:
        plt.title('Result')
    else:
        plt.clf()
        plt.title('Training...')
    plt.xlabel('Episode')
    plt.ylabel('Duration')
    plt.plot(durations_t.numpy())
    # Take 100 episode averages and plot them too
    if len(durations_t) >= 100:
        means = durations_t.unfold(0, 100, 1).mean(1).view(-1)
        means = torch.cat((torch.zeros(99), means))
        plt.plot(means.numpy())

    plt.pause(0.001)  # pause a bit so that plots are updated
    if is_ipython:
        if not show_result:
            display.display(plt.gcf())
            display.clear_output(wait=True)
        else:
            display.display(plt.gcf())
    plt.savefig('result.png')


def optimize_model():
    if len(memory) < BATCH_SIZE:
        return
    transitions = memory.sample(BATCH_SIZE)
    # Transpose the batch (see https://stackoverflow.com/a/19343/3343043 for
    # detailed explanation). This converts batch-array of Transitions
    # to Transition of batch-arrays.
    batch = Transition(*zip(*transitions))

    # Compute a mask of non-final states and concatenate the batch elements
    # (a final state would've been the one after which simulation ended)
    non_final_mask = torch.tensor(tuple(map(lambda s: s is not None,
                                            batch.next_state)), device=device, dtype=torch.bool)
    non_final_next_states = torch.cat([s for s in batch.next_state
                                       if s is not None])
    state_batch = torch.cat(batch.state)
    action_batch = torch.cat(batch.action)
    reward_batch = torch.cat(batch.reward)

    # Compute Q(s_t, a) - the model computes Q(s_t), then we select the
    # columns of actions taken. These are the actions which would've been taken
    # for each batch state according to policy_net
    state_action_values = policy_net(state_batch).gather(1, action_batch)

    # Compute V(s_{t+1}) for all next states.
    # Expected values of actions for non_final_next_states are computed based
    # on the "older" target_net; selecting their best reward with max(1)[0].
    # This is merged based on the mask, such that we'll have either the expected
    # state value or 0 in case the state was final.
    next_state_values = torch.zeros(BATCH_SIZE, device=device)
    with torch.no_grad():
        next_state_values[non_final_mask] = target_net(non_final_next_states).max(1)[0]
    # Compute the expected Q values
    expected_state_action_values = (next_state_values * GAMMA) + reward_batch

    # Compute Huber loss
    criterion = nn.SmoothL1Loss()
    loss = criterion(state_action_values, expected_state_action_values.unsqueeze(1))

    # Optimize the model
    optimizer.zero_grad()
    loss.backward()
    # In-place gradient clipping
    torch.nn.utils.clip_grad_value_(policy_net.parameters(), 100)
    optimizer.step()


# exp = Experiment(mempool_key, mem_size, 'tgax-residential', './', using_waf)      # Set up the ns-3 environment
    # exp.reset()                                             # Reset the environment
    # rl = Ns3AIRL(memblock_key, Env, Act)                    # Link the shared memory block with ns-3 script
    # pro = exp.run(setting=ns3Settings, show_output=True)    # Set and run the ns-3 script (sim.cc)
rl = ns3ai.Ns3AiMsgInterface(True, True, True, 4096,
                             "My Seg", "My Cpp to Python Msg", "My Python to Cpp Msg", "My Lockable")
assert len(rl.m_py2cpp_msg) == 0
rl.m_py2cpp_msg.resize(1)
assert len(rl.m_cpp2py_msg) == 0
rl.m_cpp2py_msg.resize(n_total)
print('Created message interface, waiting for C++ side to send initial environment...')

# thpt = 0
# hol = 0
# vrtpt = 0
times = 0
alpha = 1
beta = 5
vr_constrant = 5
vrtpt_cons = 14.7
eta = 1
# try:
while True:
    # Get current state from Python
    rl.py_recv_begin()
    if rl.py_get_finished():
        print("Finished")
        break
    throughput = 0
    for i in range(n_total):
        txNode = rl.m_cpp2py_msg[i].txNode
        # print("processing i {} txNode {}".format(i, txNode))
        for j in range(n_sta+1):
            state[j, txNode] = rl.m_cpp2py_msg[i].rxPower[j]
        # state[:, txNode] = rl.m_cpp2py_msg[i].rxPower
        if txNode % n_ap == 0:  # record mcs in BSS-0
            state[int(txNode/n_ap)][-1] = rl.m_cpp2py_msg[i].mcs
        if txNode == n_ap:     # record delay and tpt of the VR node
            vrDelay = rl.m_cpp2py_msg[i].holDelay
            vrThroughput = rl.m_cpp2py_msg[i].throughput
        # Sum all nodes' throughput
        throughput += rl.m_cpp2py_msg[i].throughput
    rl.py_recv_end()

    print("interaction count = {}, state: vr delay = {}, vr tpt = {}, total tpt = {}".format(
        times, vrDelay, vrThroughput, throughput
    ))

    cur_state = torch.tensor(state.reshape(1, -1)[0], dtype=torch.float32, device=device).unsqueeze(0)

    # RL algorithm here and put the data back to the action
    rl.py_send_begin()
    if times == 0:
        prev_state = cur_state
        action = 0
        rl.m_py2cpp_msg[0].newCcaSensitivity = -82 + action
        times += 1
    else:
        reward = alpha * throughput + beta * (vr_constrant - vrDelay) + eta * (vrThroughput - vrtpt_cons)
        rewards.append(reward)
        reward = torch.tensor([reward], device=device)
        memory.push(prev_state, action, cur_state, reward)
        prev_state = cur_state
        action = select_action(cur_state)
        rl.m_py2cpp_msg[0].newCcaSensitivity = -82 + action
        times += 1
        optimize_model()
        target_net_state_dict = target_net.state_dict()
        policy_net_state_dict = policy_net.state_dict()
        for key in policy_net_state_dict:
            target_net_state_dict[key] = policy_net_state_dict[key]*TAU + target_net_state_dict[key]*(1-TAU)
        target_net.load_state_dict(target_net_state_dict)
    print("new CCA: {}".format(rl.m_py2cpp_msg[0].newCcaSensitivity))
    rl.py_send_end()

# except Exception as e:
#     print('Something wrong')
#     print(e)
# finally:
#     del rl

plt.figure(2)
plt.title('Rewards')
plt.xlabel('Episode')
plt.ylabel('Reward')
plt.plot(rewards)
plt.savefig('rewards.png')
plt.show()
