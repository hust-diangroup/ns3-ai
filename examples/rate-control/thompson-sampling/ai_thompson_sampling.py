# Copyright (c) 2021-2023 Huazhong University of Science and Technology
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
#         Muyuan Shen <muyuan_shen@hust.edu.cn>


import copy
from typing import List
import numpy as np
import ns3ai_ratecontrol_ts_py as py_binding
from ns3ai_utils import Experiment
import sys
import traceback


class AiThompsonSamplingStation:
    _id = -1
    m_nextMode: int = 0
    m_lastMode: int = 0
    m_mcsStats: List[py_binding.ThompsonSamplingRateStats]

    def __init__(self, id=-1) -> None:
        self._id = id
        self.m_mcsStats = []

    def Decay(self, decayIdx, decay, now) -> None:
        if decayIdx >= len(self.m_mcsStats):
            print('Invalid mscStats[{}] @ {}'.format(decayIdx, self._id))
            return
        stats = self.m_mcsStats[decayIdx]
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
    m_gammaRandomVariable = np.random.RandomState()

    def __init__(self, id=-1, stream=1) -> None:
        self._id = id
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
    wifiManager: List[AiThompsonSamplingManager] = []
    wifiStation: List[AiThompsonSamplingStation] = []

    def __init__(self, msgInterface=None, stream=1) -> None:
        self.msgInterface = msgInterface
        self.default_stream = stream
        pass

    def do(self, env: py_binding.PyEnvStruct, act: py_binding.PyActStruct):
        if env.type == 0x01:  # AiThompsonSamplingWifiManager
            n_manager = len(self.wifiManager)
            self.wifiManager.append(AiThompsonSamplingManager(id=n_manager, stream=self.default_stream))
            act.managerId = n_manager

        elif env.type == 0x02:  # DoCreateStation
            n_station = len(self.wifiStation)
            # print('{} > {} new sta {}'.format(env.managerId, env.type, n_station))
            self.wifiStation.append(AiThompsonSamplingStation(id=n_station))
            act.stationId = n_station

        elif env.type == 0x03:  # InitializeStation
            sta = self.wifiStation[env.stationId]
            for i in range(64):
                if env.data.stats[i].lastDecay < 0:
                    break
                sta.m_mcsStats.append(copy.copy(env.data.stats[i]))
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
            act.stats = sta.m_mcsStats[sta.m_nextMode]
            sta.m_lastMode = sta.m_nextMode
            # print('{} > {} sta {} dv {}/{}'.
            #       format(env.managerId, env.type, env.stationId, act.res, len(sta.m_mcsStats)))

        elif env.type == 0x09:  # DoGetRtsTxVector
            sta = self.wifiStation[env.stationId]
            act.res = 0
            act.stats = sta.m_mcsStats[0]

        elif env.type == 0x0a:  # UpdateNextMode
            # print('{} > {} sta {} up {}, {}'.
            #       format(env.managerId, env.type, env.stationId, env.data.decay.decay, env.data.decay.now))
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            man.UpdateNextMode(sta, env.data.decay.decay, env.data.decay.now)
            act.stationId = env.stationId  # only for check


ns3Settings = {
    'raa': 'AiThompsonSampling',
    'nWifi': 3,
    'standard': '11ac',
    'duration': 5}

exp = Experiment("ns3ai_ratecontrol_ts", "../../../../../", py_binding, handleFinish=True)
msgInterface = exp.run(setting=ns3Settings, show_output=True)
random_stream = 100
c = AiThompsonSamplingContainer(msgInterface=msgInterface, stream=random_stream)

try:
    while True:
        c.msgInterface.PyRecvBegin()
        c.msgInterface.PySendBegin()
        if c.msgInterface.PyGetFinished():
            break
        c.do(c.msgInterface.GetCpp2PyStruct(), c.msgInterface.GetPy2CppStruct())
        c.msgInterface.PyRecvEnd()
        c.msgInterface.PySendEnd()

except Exception as e:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    print("Exception occurred: {}".format(e))
    print("Traceback:")
    traceback.print_tb(exc_traceback)
    exit(1)

else:
    pass

finally:
    print("Finally exiting...")
    del exp
