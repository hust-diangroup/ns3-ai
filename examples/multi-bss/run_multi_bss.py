import random
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
from collections import namedtuple, deque
import math
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import ns3ai_multibss_py as py_binding
from ns3ai_utils import Experiment
import sys
import traceback


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


ns3Settings = {
    'pktSize': 1500,
    'duration': 100,
    'gi': 800,
    'channelWidth': 20,
    'rng': 2,
    'apNodes': 4,
    'networkSize': 4,
    'ring': 0,
    'maxMpdus': 5,
    'autoMCS': True,
    'prop': 'tgax',
    'app': 'setup-done',
    'pktInterval': 5000,
    'boxsize': 25,
    'drl': True,
    'configFile': 'contrib/ai/examples/multi-bss/config.txt',
}
n_ap = int(ns3Settings['apNodes'])
n_sta = int(ns3Settings['networkSize'])
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

times = 0
alpha = 1
beta = 5
vr_constrant = 5
vrtpt_cons = 14.7
eta = 1

exp = Experiment("ns3ai_multibss", "../../../../", py_binding,
                 handleFinish=True, useVector=True, vectorSize=n_total)
msgInterface = exp.run(setting=ns3Settings, show_output=True)

try:
    while True:
        # Get current state from C++
        msgInterface.PyRecvBegin()
        if msgInterface.PyGetFinished():
            print("Finished")
            break
        throughput = 0
        for i in range(n_total):
            txNode = msgInterface.GetCpp2PyVector()[i].txNode
            # print("processing i {} txNode {}".format(i, txNode))
            for j in range(n_sta+1):
                state[j, txNode] = msgInterface.GetCpp2PyVector()[i].rxPower[j]
            # state[:, txNode] = msgInterface.GetCpp2PyVector()[i].rxPower
            if txNode % n_ap == 0:  # record mcs in BSS-0
                state[int(txNode/n_ap)][-1] = msgInterface.GetCpp2PyVector()[i].mcs
            if txNode == n_ap:     # record delay and tpt of the VR node
                vrDelay = msgInterface.GetCpp2PyVector()[i].holDelay
                vrThroughput = msgInterface.GetCpp2PyVector()[i].throughput
            # Sum all nodes' throughput
            throughput += msgInterface.GetCpp2PyVector()[i].throughput
        msgInterface.PyRecvEnd()

        print("step = {}, VR avg delay = {} ms, VR UL tpt = {} Mbps, total UL tpt = {} Mbps".format(
            times, vrDelay, vrThroughput, throughput
        ))

        # RL algorithm here, select action
        cur_state = torch.tensor(state.reshape(1, -1)[0], dtype=torch.float32, device=device).unsqueeze(0)
        if times == 0:
            prev_state = cur_state
            action = torch.tensor([[0]], device=device, dtype=torch.long)
        else:
            reward = alpha * throughput + beta * (vr_constrant - vrDelay) + eta * (vrThroughput - vrtpt_cons)
            rewards.append(reward)
            reward = torch.tensor([reward], device=device)
            memory.push(prev_state, action, cur_state, reward)
            prev_state = cur_state
            action = select_action(cur_state)
            optimize_model()
            target_net_state_dict = target_net.state_dict()
            policy_net_state_dict = policy_net.state_dict()
            for key in policy_net_state_dict:
                target_net_state_dict[key] = policy_net_state_dict[key]*TAU + target_net_state_dict[key]*(1-TAU)
            target_net.load_state_dict(target_net_state_dict)

        # put the action back to C++
        msgInterface.PySendBegin()
        msgInterface.GetPy2CppVector()[0].newCcaSensitivity = -82 + action
        msgInterface.PySendEnd()
        print("new CCA: {}".format(msgInterface.GetPy2CppVector()[0].newCcaSensitivity))
        times += 1

except Exception as e:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    print("Exception occurred: {}".format(e))
    print("Traceback:")
    traceback.print_tb(exc_traceback)
    exit(1)

else:
    plt.figure(2)
    plt.title('Rewards')
    plt.xlabel('Episode')
    plt.ylabel('Reward')
    plt.plot(rewards)
    plt.savefig('rewards.png')
    plt.show()

finally:
    print("Finally exiting...")
    del exp
