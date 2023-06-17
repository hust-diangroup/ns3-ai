#! /usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
# Copyright (c) 2021 Huazhong University of Science and Technology, Dian Group
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
# Author: Xun Deng <dorence@hust.edu.cn>
#         Hao Yin <haoyin@uw.edu>

import copy
# from ctypes import *
# from typing import List
# import numpy as np
# from py_interface import *

from typing import List
import numpy as np

import sys
sys.path.append("../../../utils")
import ns3_util
import ns3ai_ratecontrol_ts_py as ts

class AiThompsonSamplingStation:
    _id = -1
    # _addr = 0
    m_nextMode: int = 0
    m_lastMode: int = 0
    m_mcsStats: List[ts.ThompsonSamplingRateStats]

    # def __init__(self, id=-1, addr=0) -> None:
    def __init__(self, id=-1) -> None:
        self._id = id
        # self._addr = addr
        self.m_mcsStats = []

    def Decay(self, decayIdx, decay, now) -> None:
        if decayIdx >= len(self.m_mcsStats):
            print('Invalid mscStats[{}] @ {}'.format(decayIdx, self._id))
            return
        stats = self.m_mcsStats[decayIdx]
        # print('Decay', decayIdx, now, stats.lastDecay, len(self.m_mcsStats))
        if now > stats.lastDecay:
            coefficient = np.exp(decay * (stats.lastDecay - now))
            stats.success = coefficient * stats.success
            stats.fails = coefficient * stats.fails
            stats.lastDecay = now

    def DoReportDataFailed(self, decay, now) -> None:
        idx = self.m_lastMode
        self.Decay(idx, decay, now)
        self.m_mcsStats[idx].fails = self.m_mcsStats[idx].fails + 1

    def DoReportDataOk(self, decay, now) -> None:
        idx = self.m_lastMode
        self.Decay(idx, decay, now)
        self.m_mcsStats[idx].success = self.m_mcsStats[idx].success + 1

    def DoReportAmpduTxStatus(self, decay, now, successful, failed) -> None:
        idx = self.m_lastMode
        self.Decay(idx, decay, now)
        self.m_mcsStats[idx].fails = self.m_mcsStats[idx].fails + failed
        self.m_mcsStats[idx].success = self.m_mcsStats[idx].success + successful

    pass


class AiThompsonSamplingManager:
    _id = -1
    # _addr = 0
    m_gammaRandomVariable = np.random.RandomState()

    # def __init__(self, id=-1, stream=1, addr=0) -> None:
    def __init__(self, id=-1, stream=1) -> None:
        self._id = id
        # self._addr = addr
        self.m_gammaRandomVariable = np.random.RandomState(seed=stream)

    def SampleBetaVariable(self, alpha, beta):
        X = self.m_gammaRandomVariable.gamma(alpha, 1.0)
        Y = self.m_gammaRandomVariable.gamma(beta, 1.0)
        return X / (X + Y)

    def UpdateNextMode(self, station: AiThompsonSamplingStation, decay, now):
        maxThroughput = 0.0
        frameSuccessRate = 1.0
        station.m_nextMode = 0
        for i in range(len(station.m_mcsStats)):
            station.Decay(i, decay, now)
            frameSuccessRate = self.SampleBetaVariable(
                1.0 + station.m_mcsStats[i].success,
                1.0 + station.m_mcsStats[i].fails
            )
            rate = station.m_mcsStats[i].dataRate
            if (frameSuccessRate * rate > maxThroughput):
                maxThroughput = frameSuccessRate * rate
                station.m_nextMode = i
        pass


class AiThompsonSamplingContainer:
    use_ns3ai = True
    # AiThompsonSamplingManager
    wifiManager: List[AiThompsonSamplingManager] = []
    # AiThompsonSamplingStation
    wifiStation: List[AiThompsonSamplingStation] = []

    def __init__(self, uid=2333, stream=1) -> None:
        self.rl = ts.NS3AIRL(4096, True, "My Seg", "My Env", "My Act", "My Lockable")
        self.default_stream = stream
        # print('({})size: Env {} Act {}'.format(uid, sizeof(AiThompsonSamplingEnv), sizeof(AiThompsonSamplingAct)))
        pass

    def __del__(self):
        del self.rl

    # get action
    # def do(self, env: ts.PyEnvStruct, act: ts.PyActStruct) -> ts.PyActStruct:
    def do(self, env: ts.PyEnvStruct, act: ts.PyActStruct):
        if env.type == 0x01:  # AiThompsonSamplingWifiManager
            n_manager = len(self.wifiManager)
            # self.wifiManager.append(AiThompsonSamplingManager(addr=env.addr, id=n_manager, stream=self.default_stream))
            self.wifiManager.append(AiThompsonSamplingManager(id=n_manager, stream=self.default_stream))
            act.managerId = n_manager

        elif env.type == 0x02:  # DoCreateStation
            n_station = len(self.wifiStation)
            # print('{} > {} new sta {} @ {}'.format(env.managerId, env.type, n_station, env.addr))
            # print('{} > {} new sta {}'.format(env.managerId, env.type, n_station))
            # self.wifiStation.append(AiThompsonSamplingStation(addr=env.addr, id=n_station))
            self.wifiStation.append(AiThompsonSamplingStation(id=n_station))
            act.stationId = n_station

        elif env.type == 0x03:  # InitializeStation
            sta = self.wifiStation[env.stationId]
            for i in range(64):
                if env.data.stats[i].lastDecay < 0:
                    break
                sta.m_mcsStats.append(copy.copy(env.data.stats[i]))
                # print("SMY: in do, appending mcsstats: get i={}, nss={}, channel width={}, interval={}".
                #       format(i, env.data.stats[i].nss, env.data.stats[i].channelWidth, env.data.stats[i].guardInterval))
            # print('{} > {} sta {} msc {}'.format(env.managerId, env.type, env.stationId, len(sta.m_mcsStats)))
            act.stationId = env.stationId  # only for check

        elif env.type == 0x04:  # Decay
            # print('{} > {} sta {}/{}'.format(env.managerId, env.type, env.stationId, len(self.wifiStation)))
            sta = self.wifiStation[env.stationId]
            sta.Decay(env.data.decay.decayIdx, env.data.decay.decay, env.data.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x05:  # DoReportDataFailed
            # print('{} > {} sta {} failed'.format(env.managerId, env.type, env.stationId))
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            sta.DoReportDataFailed(env.data.decay.decay, env.data.decay.now)
            man.UpdateNextMode(sta, env.data.decay.decay, env.data.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x06:  # DoReportDataOk
            # print('{} > {} sta {} ok'.format(env.managerId, env.type, env.stationId))
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            sta.DoReportDataOk(env.data.decay.decay, env.data.decay.now)
            man.UpdateNextMode(sta, env.data.decay.decay, env.data.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x07:  # DoReportAmpduTxStatus
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            successful = env.var >> 32
            failed = env.var & 0xffffffff
            # print('{} > {} sta {} ampdu {}/{}'.format(env.managerId, env.type, env.stationId, successful, failed))
            sta.DoReportAmpduTxStatus(env.data.decay.decay, env.data.decay.now, successful, failed)
            man.UpdateNextMode(sta, env.data.decay.decay, env.data.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x08:  # DoGetDataTxVector
            sta = self.wifiStation[env.stationId]
            act.res = sta.m_nextMode
            # print("SMY: in do: act.res set to next mode, which is {}".format(act.res))
            act.stats = sta.m_mcsStats[sta.m_nextMode]
            # print("SMY: in do: act.stats.nss set to {}".format(act.stats.nss))
            sta.m_lastMode = sta.m_nextMode
            # print('{} > {} sta {} dv {}/{}'.
            #       format(env.managerId, env.type, env.stationId, act.res, len(sta.m_mcsStats)))

        elif env.type == 0x09:  # DoGetRtsTxVector
            sta = self.wifiStation[env.stationId]
            act.res = 0
            # print("SMY: in do: act.res set to 0")
            act.stats = sta.m_mcsStats[0]
            # print("SMY: in do: act.stats.nss set to {}".format(act.stats.nss))

        elif env.type == 0x0a:  # UpdateNextMode
            # print('{} > {} sta {} up {}, {}'.
            #       format(env.managerId, env.type, env.stationId, env.data.decay.decay, env.data.decay.now))
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            man.UpdateNextMode(sta, env.data.decay.decay, env.data.decay.now)
            act.stationId = env.stationId  # only for check

        # return act


if __name__ == '__main__':
    # ns3Settings = {'raa': 'AiThompsonSampling', 'nWifi': 3, 'standard': '11ac', 'duration': 5}
    # mempool_key = 1234 # memory pool key, arbitrary integer large than 1000
    # mem_size = 4096 # memory pool size in bytes
    # exp = Experiment(mempool_key, mem_size, 'rate-control', '../../', using_waf=False)
    # exp.reset()

    # memblock_key = 2333 # memory block key in the memory pool, arbitrary integer, and need to keep the same in the ns-3 script
    random_stream = 100
    c = AiThompsonSamplingContainer(stream=random_stream)

    # myenvs = ts.PyEnvVector()
    # myacts = ts.PyActVector()
    # temp_env = ts.PyEnvStruct()
    # temp_act = ts.PyActStruct()

    # pro = exp.run(setting=ns3Settings, show_output=True)
    # print("run rate-control", ns3Settings)
    while True:
        # myacts.clear()
        # c.rl.get_env(myenvs)
        c.rl.get_env_begin()
        c.rl.set_act_begin()
        if c.rl.is_finished():
            break
        # temp_env = myenvs[0]
        # temp_act = c.do(temp_env, temp_act)
        c.do(c.rl.m_env[0], c.rl.m_act[0])
        c.rl.get_env_end()
        c.rl.set_act_end()
        # myacts.append(temp_act)
        # print("SMY: before set_act: act.res set to {}".format(myacts[0].res))
        # c.rl.set_act(myacts)

    del c
