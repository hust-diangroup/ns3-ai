#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tensorflow as tf
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from tensorflow import keras
from py_interface import *
from ctypes import *
import sys

tf.random.set_seed(0)
np.random.seed(0)

MAX_CHANNEL_NUM = 64


class sEnv(Structure):
    # _pack_ = 1
    _fields_ = [
        ('reward', c_float),
        ('done', c_bool),
        ('channNum', c_uint32),
        ('channelOccupation', c_uint32*MAX_CHANNEL_NUM),
    ]


class sAct(Structure):
    # _pack_ = 1
    _fields_ = [
        ('nextChannel', c_uint32),
    ]


class sInfo(Structure):
    # _pack_ = 1
    _fields_ = [
        ('channelNum', c_uint32),
    ]


exp = Experiment(1234, 4096, 'interference-pattern', '../../')
var = Ns3AIRL(2333, sEnv, sAct, sInfo)

model = keras.Sequential()
model.add(keras.layers.Dense(4, input_shape=(4,), activation='tanh'))
model.compile(optimizer='adam',
              loss='mean_squared_error',
              metrics=['accuracy'])

total_episodes = 20
max_env_steps = 100

epsilon = 1.0
epsilon_min = 0.01
epsilon_decay = 0.99

time_history = []
rew_history = []
dec_arr = np.array([[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [
                   0, 0, 0, 1]], dtype=np.float64)
train = 3
try:
    for e in range(total_episodes):
        exp.reset()
        exp.run(show_output=0)
        state = np.array([1, 0, 0, 0])
        state = np.reshape(state, [1, 4])
        rewardsum = 0
        lst_act = 0
        for time in range(max_env_steps):
            if np.random.rand(1) < epsilon:
                action = np.random.randint(0, 4)
            else:
                res = model.predict(state)[0]
                action = np.argmax(res)

            with var as data:
                if data == None:
                    break
                state = data.env.channelOccupation[:data.env.channNum]
                reward = int(data.env.reward)
                done = data.env.done
                data.act.nextChannel = c_uint32(action)
            state = np.array(state)
            state = np.reshape(state, [1, 4])

            if done:
                break

            target = model.predict(state)[0]-reward * \
                dec_arr[lst_act & 3:(lst_act & 3)+1]
            target -= np.mean(target)
            target /= np.std(target)
            if reward < 0:
                epoch = 10
            else:
                epoch = 5
            if train > 0:
                model.fit(state, target.reshape((1, 4)), epochs=epoch, verbose=0)

            lst_act = action
            rewardsum += reward
            if epsilon > epsilon_min:
                epsilon *= epsilon_decay

        if rewardsum >= 90:
            if train>0:
                train -= 1
        else:
            train = 3
        print("episode: {}/{}, time: {}, rew: {}"
            .format(e, total_episodes, time, rewardsum))

        time_history.append(time)
        rew_history.append(rewardsum)
        exp.kill()
    print("Plot Learning Performance")
    mpl.rcdefaults()
    mpl.rcParams.update({'font.size': 16})

    fig, ax = plt.subplots(figsize=(10, 4))
    plt.grid(True, linestyle='--')
    plt.title('Learning Performance')
    plt.plot(range(len(time_history)), time_history, label='Steps',
            marker="^", linestyle=":")  # , color='red')
    plt.plot(range(len(rew_history)), rew_history, label='Reward',
            marker="", linestyle="-")  # , color='k')
    plt.xlabel('Episode')
    plt.ylabel('Time')
    plt.legend(prop={'size': 12})

    plt.savefig('learning.pdf', bbox_inches='tight')
    plt.show()
except Exception as e:
    print('Something wrong')
    print(e)
finally:
    del exp