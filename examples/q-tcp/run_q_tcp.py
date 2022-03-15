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
# Modified: Shayekh Bin Islam <shayekh.bin.islam@gmail.com>


# use_wandb = False
# use_wandb = False
# if use_wandb:
#   import wandb
#   wandb.init(project="", entity="")

import copy
import random

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
    ('lastRtt', c_int64),
    ('ackArrivalTime', c_double),
    ('sendTime', c_double),
    ('rttRatio', c_double),
    ('delay', c_uint64),
    ('throughput', c_uint64),
  ]


class TcpRlAct(Structure):
  _pack_ = 1
  _fields_ = [
    ('new_ssThresh', c_uint32),
    ('new_cWnd', c_uint32)
  ]


discretization_level = 15

def discretize(metric, minval, maxval, level):
  metric = max(metric, minval)
  metric = min(metric, maxval)
  return int((metric - minval)*(level-1)/ (maxval - minval))


def discretize_rttRatio(rttRatio):
  return discretize(rttRatio, 1., 1.5, discretization_level)

def discretize_sendTime(sendTime):
  return discretize(sendTime, 0., 1000.0, discretization_level)

def discretize_ackArrivalTime(ackArrivalTime):
  return discretize(ackArrivalTime, 0., 500.0, discretization_level)

def discretize_cWnd(cWnd):
  return discretize(cWnd, 0., 1000.0, discretization_level)

def discretize_ssThresh(ssThresh):
  return discretize(ssThresh, 0., 500.0, discretization_level)



ssThresh = 0
cWnd = 0
segmentsAcked = 0
segmentSize = 0
bytesInFlight = 0

# https://www.learndatasci.com/tutorials/reinforcement-q-learning-scratch-python-openai-gym/

def main():
  global ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight
  import numpy as np
# if 1:
  Init(1234, 4096)
  var = Ns3AIRL(1234, TcpRlEnv, TcpRlAct)
  res_list = ['ssThresh_l', 'cWnd_l', 'segmentsAcked_l', 'segmentSize_l', 'bytesInFlight_l']
  args = parser.parse_args()
  # if use_wandb:
  #   # wandb.config = dict(args)
  #   # wandb.config = vars(args)
  #   # wandb.config.update(args)
  #   if args.use_rl:
  #     wandb.log({"algo": 1}) # DQN
  #   else:
  #     wandb.log({"algo": 0}) # Reno
  #   # print(vars(args))
  #   # exit()

  if args.result:
    for res in res_list:
      globals()[res] = []
    if args.output_dir:
      if not os.path.exists(args.output_dir):
        os.mkdir(args.output_dir)

  if args.use_rl:
    # dqn = DQN()
    # wandb.watch(dqn.eval_net)
    # wandb.watch(dqn.target_net)
    actions = np.array([-1, 0, 1, 3])
    q_table = np.zeros([4, *((discretization_level,)*5), actions.shape[0]])
    # print(q_table.shape); exit()

  exp = Experiment(1234, 4096, 'q-tcp', '../../')
  exp.run(show_output=1)

  timestep = 0
  act2 = [0,]*10
  act1 = [0,]*10
  act0 = [0,]*10
  state0 = [(0, 0, 0, 0, 0),]*10
  state1 = [(0, 0, 0, 0, 0),]*10
  state2 = [(0, 0, 0, 0, 0),]*10
  util1 = [0,]*10
  util2 = [0,]*10
  updatetime = [0,]*10

  def utility(throughput_c, delay_c):
    import math

    # print(throughput_c, delay_c)
    r = 0
    if throughput_c > 0:
      r += math.log(throughput_c/1e7)
    if delay_c > 0:
      # r += -0.2 * math.log(delay_c);
      r += -0.01 * math.log(delay_c);
    
    return r
  
  def reward(u2, u1):
    diff = u2 - u1
    if diff >= 1:
      return 10
    elif 0<= diff < 1:
      return 2
    elif -1 <= diff < 0:
      return -2
    else:
      return -10

  try:
    while not var.isFinish():
      with var as data:
        if not data:
          break
    #         print(var.GetVersion())
        nodeId = data.env.nodeId
        segmentsAcked = data.env.segmentsAcked
        segmentSize = data.env.segmentSize
        bytesInFlight = data.env.bytesInFlight
        lastRtt = data.env.lastRtt
        simTime_us = data.env.simTime_us

        ssThresh = data.env.ssThresh
        cWnd = data.env.cWnd
        ackArrivalTime = data.env.ackArrivalTime
        sendTime = data.env.sendTime
        rttRatio = data.env.rttRatio
        delay = data.env.delay
        throughput = data.env.throughput

        print(f"nodeId: {nodeId}, cWnd: {cWnd}")
    #         print(ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight)

        if args.result:
          for res in res_list:
            globals()[res].append(globals()[res[:-2]])
            #print(globals()[res][-1])

        if not args.use_rl:
          # TCP-newreno
          # https://www.nsnam.org/doxygen/tcp-congestion-ops_8cc_source.html#l00188
          # From tcp-congestion-ops.cc
          # Functions: 
          # new_cWnd = cWnd
          # new_ssThresh = ssThresh
          # new_cWnd = 1
          # new_ssThresh = 1
          # IncreaseWindow: TcpNewReno::IncreaseWindow 
          # print("cwnd: {}, ssThresh: {}".format(cWnd, ssThresh))
          if (cWnd < ssThresh):
            # slow start
            # print("slow start outer\n")
            if (segmentsAcked >= 1):
              new_cWnd = cWnd + segmentSize
              # print("slow start inner\n")
          if (cWnd >= ssThresh):
            # congestion avoidance
            # print("congestion avoidance outer\n")
            if (segmentsAcked > 0):
              # print("congestion avoidance inner\n")
              adder = 1.0 * (segmentSize * segmentSize) / cWnd
              adder = int(max(1.0, adder))
              new_cWnd = cWnd + adder
          # GetSsThresh: TcpNewReno::GetSsThresh 
          new_ssThresh = int(max(2 * segmentSize, bytesInFlight / 2))
          # print("cwnd from {} to {}\n".format(cWnd, new_cWnd))
          data.act.new_cWnd = new_cWnd
          # print("new_ssThresh from {} to {}\n".format(ssThresh, new_ssThresh))
          data.act.new_ssThresh = new_ssThresh
          
          cWnd_c = copy.deepcopy(cWnd)
          # new_cWnd_c = copy.deepcopy(new_cWnd)
          segmentSize_c = copy.deepcopy(segmentSize)
          ssThresh_c = copy.deepcopy(ssThresh)
          # simTime_us_c = copy.deepcopy(simTime_us)
          rttRatio_c = copy.deepcopy(rttRatio)
          ackArrivalTime_c = copy.deepcopy(ackArrivalTime)
          sendTime_c = copy.deepcopy(sendTime)
          delay_c = copy.deepcopy(delay)
          throughput_c = copy.deepcopy(throughput)
          nodeId_c = copy.deepcopy(nodeId)

          
          # if use_wandb:
          #   # if nodeId == 2:
          #   #   wandb.log({
          #   #     "cWnd": cWnd_c / segmentSize_c, 
          #   #     "new_cWnd": new_cWnd_c/ segmentSize_c,
          #   #     "ssThresh": ssThresh_c, 
          #   #     "simTime_us": simTime_us,
          #   #     # "new_ssThresh": new_ssThresh,
          #   #   })

          #   # wandb.log({
          #   #   f"cWnd_{nodeId}": cWnd_c / segmentSize_c,
          #   #   "simTime_us": simTime_us,
          #   # })

          #   if 1:
          #     wandb.log({
          #       "cWnd_{}".format(nodeId_c): cWnd_c / 1250, 
          #       # "new_cWnd": new_cWnd_c/ segmentSize_c,
          #       # "ssThresh_{}".format(nodeId_c): ssThresh_c if ssThresh_c < 1e6 else 1e6, 
          #       "ssThresh_{}".format(nodeId_c): ssThresh_c / 1250, 
          #       # "new_ssThresh": new_ssThresh,
          #       # "nodeId": nodeId_c,
          #     })

        else:
          cWnd_c = copy.deepcopy(cWnd)
          # new_cWnd_c = copy.deepcopy(new_cWnd)
          segmentSize_c = copy.deepcopy(segmentSize)
          ssThresh_c = copy.deepcopy(ssThresh)
          # simTime_us_c = copy.deepcopy(simTime_us)
          rttRatio_c = copy.deepcopy(rttRatio)
          ackArrivalTime_c = copy.deepcopy(ackArrivalTime)
          sendTime_c = copy.deepcopy(sendTime)
          nodeId_c = copy.deepcopy(nodeId)
          delay_c = copy.deepcopy(delay)
          throughput_c = copy.deepcopy(throughput)


          # new_cWnd = 1
          # new_ssThresh = 1
          # s = [ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight]
          # a = dqn.choose_action(s)
          # print(cWnd_c)
          # exit()

          state2[nodeId_c] = (discretize_cWnd(cWnd_c/segmentSize_c), discretize_ssThresh(ssThresh_c/segmentSize_c),
            discretize_rttRatio(rttRatio_c), discretize_ackArrivalTime(ackArrivalTime_c),
            discretize_sendTime(sendTime_c)
          )
          print(state2[nodeId_c], flush=True)
          util2[nodeId_c] = utility(throughput_c, delay_c)
          r2 = reward(util2[nodeId_c], util1[nodeId_c])
          # print(q_table[(1, 1, 0, 0, 11)])
          # print(q_table[state2])
          update_q = False
          

          # Update new_cWnd
          if (cWnd < ssThresh):
            # slow start
            # print("slow start outer\n")
            new_cWnd = cWnd
            if (segmentsAcked >= 1):
              new_cWnd = cWnd + segmentSize
              # print("slow start inner\n")
          else:
            if (segmentsAcked >= 1):
              if simTime_us - timestep >= lastRtt:
                if random.uniform(0, 1) < 0.1:
                  act = np.random.choice([0, 1, 2, 3], size=1)[0]
                else:
                  act = np.argmax(q_table[nodeId_c-2][state2])
                
                print("act: {}".format(act))
                act2[nodeId_c] = actions[act]
                new_cWnd = int(cWnd + act2[nodeId_c] / cWnd)

                # old_value = q_table[nodeId_c-2][state0[nodeId_c]][act0[nodeId_c]]
                # act2[nodeId_c] = np.max(q_table[nodeId_c-2][state1[nodeId_c]])
                gamma = 0.5
                alpha = 0.3 * (0.995**(updatetime[nodeId_c] // 10))
                updatetime[nodeId_c] += 1

                q_table[nodeId_c-2][state0[nodeId_c]][act0[nodeId_c]] = (
                  (1 - alpha) * q_table[nodeId_c-2][state0[nodeId_c]][act0[nodeId_c]] + 
                  alpha * (r2  + gamma * q_table[nodeId_c-2][state1[nodeId_c]][act1[nodeId_c]])
                )

                act0[nodeId_c] = act1[nodeId_c]
                act1[nodeId_c] = act2[nodeId_c]
                util1[nodeId_c] = util2[nodeId_c]
                state0[nodeId_c] = state1[nodeId_c]
                state1[nodeId_c] = state2[nodeId_c]
                timestep = simTime_us
                # update_q = True
              else:              
                new_cWnd = int(cWnd + act1[nodeId_c] / cWnd)
          
          # elif a & 1:
          #   new_cWnd = cWnd + segmentSize
          # else:
          #   if(cWnd > 0):
          #     new_cWnd = cWnd + int(max(1, (segmentSize * segmentSize) / cWnd))

          # update new_ssThresh
          # if a < 3:
          #   new_ssThresh = 2 * segmentSize
          # else:
          #   new_ssThresh = int(bytesInFlight / 2)

          # GetSsThresh: TcpNewReno::GetSsThresh 
          new_ssThresh = int(max(2 * segmentSize, bytesInFlight / 2))

          data.act.new_cWnd = new_cWnd
          data.act.new_ssThresh = new_ssThresh

          nodeId = data.env.nodeId
          segmentsAcked = data.env.segmentsAcked
          segmentSize = data.env.segmentSize
          bytesInFlight = data.env.bytesInFlight
          lastRtt = data.env.lastRtt
          simTime_us = data.env.simTime_us
          ssThresh = data.env.ssThresh
          cWnd = data.env.cWnd
          ackArrivalTime = data.env.ackArrivalTime
          sendTime = data.env.sendTime
          rttRatio = data.env.rttRatio
          delay = data.env.delay
          throughput = data.env.throughput

          cWnd_c = copy.deepcopy(cWnd)
          # new_cWnd_c = copy.deepcopy(new_cWnd)
          segmentSize_c = copy.deepcopy(segmentSize)
          ssThresh_c = copy.deepcopy(ssThresh)
          # simTime_us_c = copy.deepcopy(simTime_us)
          rttRatio_c = copy.deepcopy(rttRatio)
          ackArrivalTime_c = copy.deepcopy(ackArrivalTime)
          sendTime_c = copy.deepcopy(sendTime)
          nodeId_c = copy.deepcopy(nodeId)
          delay_c = copy.deepcopy(delay)
          throughput_c = copy.deepcopy(throughput)

          # if update_q:
          

            # ssThresh = data.env.ssThresh
            # cWnd = data.env.cWnd
            # segmentsAcked = data.env.segmentsAcked
            # segmentSize = data.env.segmentSize
            # bytesInFlight = data.env.bytesInFlight
            # new_lastRtt = data.env.lastRtt

            # print("Rtt updated from {} to {}".format(lastRtt, new_lastRtt))

            # modify the reward
            # r = segmentsAcked - bytesInFlight - cWnd
            # r = segmentsAcked - bytesInFlight - cWnd
            # r = -lastRtt

            # import math
            # print(throughput_c, delay_c)
            # r = 0
            # if throughput_c > 0:
            #   r += math.log(throughput_c/1e6)
            # if delay_c > 0:
            #   r += -0.01 * math.log(delay_c);
          
            # r = math.log()
            # s_ = [ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight]
            # dqn.store_transition(s, a, r, s_)

            # s3 = (discretize_cWnd(cWnd_c/segmentSize_c), discretize_ssThresh(ssThresh_c/segmentSize_c),
            #   discretize_rttRatio(rttRatio_c), discretize_ackArrivalTime(ackArrivalTime_c),
            #   discretize_sendTime(sendTime_c)
            # )

            # print(nodeId_c)


          # if use_wandb:
          #   # if nodeId == 2:
          #   if 1:
          #     wandb.log({
          #       "cWnd_{}".format(nodeId_c): cWnd_c / 1250, 
          #       # "new_cWnd": new_cWnd_c/ segmentSize_c,
          #       # "ssThresh_{}".format(nodeId_c): ssThresh_c if ssThresh_c < 1e6 else 1e6, 
          #       "ssThresh_{}".format(nodeId_c): ssThresh_c / 1250, 
          #       # "new_ssThresh": new_ssThresh,
          #       # "nodeId": nodeId_c,
          #     })



          # if dqn.memory_counter > dqn.memory_capacity:
            # print("Training dqn")
            # dqn.learn()

  except KeyboardInterrupt:
    exp.kill()
    del exp
    exit() # avoid plotting

  if args.result:
    import numpy as np
    segsizes = np.array(globals()['segmentSize_l'])

    for res in res_list:
      y = globals()[res]
      x = range(len(y))
      if res == 'cWnd_l':
        y = np.array(y) / 1250
      
      if res == 'ssThresh_l':
        y = list(filter(lambda x: x < 1e9, y))
        y = np.array(y) / 1250
        x = range(len(y))


      plt.clf()
      # if res == 'cWnd_l':
      #   plt.plot(x, y, label=res[:-2], linewidth=1, color='r')
      # else:
      #   plt.plot(x, y, label=res[:-2], linewidth=1, color='r')
      
      plt.plot(x, y, label=res[:-2], linewidth=1, color='r')
      
      plt.xlabel('Step Number')
      if res == 'cWnd_l':
        plt.ylabel('Segments')
      
      plt.title('Information of {}'.format(res[:-2]))
      plt.savefig('{}.png'.format(os.path.join(args.output_dir, res[:-2])))

if __name__ == '__main__':
  main()
    
