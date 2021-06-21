#! /usr/bin/env python
# -*- coding: utf-8 -*-

import copy
from ctypes import *
from typing import List

import numpy as np
from py_interface import *


class ThompsonSamplingRateStats(Structure):
    _pack_ = 1
    _fields_ = [
        ('nss', c_uint8),
        ('channelWidth', c_uint16),
        ('guardInterval', c_uint16),
        ('dataRate', c_uint64),
        ('success', c_double),
        ('fails', c_double),
        ('lastDecay', c_double)  # Time
    ]


class AiThompsonSamplingEnvDecay(Structure):
    _pack_ = 1
    _fields_ = [
        ('decayIdx', c_int8),
        ('decay', c_double),
        ('now', c_double)  # Time
    ]


class AiThompsonSamplingEnvPayload(Union):
    _pack_ = 1
    _fields_ = [
        ('addr', c_void_p),
        ('stats', ThompsonSamplingRateStats * 64),
        ('decay', AiThompsonSamplingEnvDecay)
    ]


class AiThompsonSamplingEnv(Structure):
    _pack_ = 1
    _anonymous_ = ['data']
    _fields_ = [
        ('type', c_int8),
        ('managerId', c_int8),
        ('stationId', c_int8),
        ('var', c_uint64),
        ('data', AiThompsonSamplingEnvPayload)
    ]


class AiThompsonSamplingAct(Structure):
    _pack_ = 1
    _fields_ = [
        ('managerId', c_int8),
        ('stationId', c_int8),
        ('res', c_uint64),
        ('stats', ThompsonSamplingRateStats)
    ]


class AiThompsonSamplingStation:
    _id = -1
    _addr = 0
    m_nextMode: int = 0
    m_lastMode: int = 0
    m_mcsStats: List[ThompsonSamplingRateStats]

    def __init__(self, id=-1, addr=0) -> None:
        self._id = id
        self._addr = addr
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
    _addr = 0
    m_gammaRandomVariable = np.random.RandomState()

    def __init__(self, id=-1, stream=1, addr=0) -> None:
        self._id = id
        self._addr = addr
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
        self.rl = Ns3AIRL(uid, AiThompsonSamplingEnv, AiThompsonSamplingAct)
        self.default_stream = stream
        print('({})size: Env {} Act {}'.format(uid, sizeof(AiThompsonSamplingEnv), sizeof(AiThompsonSamplingAct)))
        pass

    def __del__(self):
        if not self.rl.finished:
            self.rl.Release()

    def do(self, env: AiThompsonSamplingEnv, act: AiThompsonSamplingAct) -> AiThompsonSamplingAct:
        if env.type == 0x01:  # AiThompsonSamplingWifiManager
            id = len(self.wifiManager)
            self.wifiManager.append(AiThompsonSamplingManager(addr=env.addr, id=id, stream=self.default_stream))
            act.managerId = c_int8(id)

        elif env.type == 0x02:  # DoCreateStation
            id = len(self.wifiStation)
            # print('{} > {} new sta {} @ {}'.format(env.managerId, env.type, id, env.addr))
            self.wifiStation.append(AiThompsonSamplingStation(addr=env.addr, id=id))
            act.stationId = c_int8(id)

        elif env.type == 0x03:  # InitializeStation
            sta = self.wifiStation[env.stationId]
            for i in range(64):
                if env.stats[i].lastDecay < 0:
                    break
                sta.m_mcsStats.append(copy.deepcopy(env.stats[i]))
            # print('{} > {} sta {} msc {}'.format(env.managerId, env.type, env.stationId, len(sta.m_mcsStats)))
            act.stationId = env.stationId  # only for check

        elif env.type == 0x04:  # Decay
            # print('{} > {} sta {}/{}'.format(env.managerId, env.type, env.stationId, len(self.wifiStation)))
            sta = self.wifiStation[env.stationId]
            sta.Decay(env.decay.decayIdx, env.decay.decay, env.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x05:  # DoReportDataFailed
            # print('{} > {} sta {} failed'.format(env.managerId, env.type, env.stationId))
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            sta.DoReportDataFailed(env.decay.decay, env.decay.now)
            man.UpdateNextMode(sta, env.decay.decay, env.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x06:  # DoReportDataOk
            # print('{} > {} sta {} ok'.format(env.managerId, env.type, env.stationId))
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            sta.DoReportDataOk(env.decay.decay, env.decay.now)
            man.UpdateNextMode(sta, env.decay.decay, env.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x07:  # DoReportAmpduTxStatus
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            successful = env.var >> 32
            failed = env.var & 0xffffffff
            # print('{} > {} sta {} ampdu {}/{}'.format(env.managerId, env.type, env.stationId, successful, failed))
            sta.DoReportAmpduTxStatus(env.decay.decay, env.decay.now, successful, failed)
            man.UpdateNextMode(sta, env.decay.decay, env.decay.now)
            act.stationId = env.stationId  # only for check

        elif env.type == 0x08:  # DoGetDataTxVector
            sta = self.wifiStation[env.stationId]
            act.res = sta.m_nextMode
            act.stats = sta.m_mcsStats[sta.m_nextMode]
            sta.m_lastMode = sta.m_nextMode
            # print('{} > {} sta {} dv {}/{} {}'.format(env.managerId, env.type, env.stationId, act.res, len(sta.m_mcsStats)))

        elif env.type == 0x09:  # DoGetRtsTxVector
            sta = self.wifiStation[env.stationId]
            act.res = 0
            act.stats = sta.m_mcsStats[0]

        elif env.type == 0x0a:  # UpdateNextMode
            # print('{} > {} sta {} up {}, {}'.format(env.managerId, env.type, env.stationId, env.decay.decay, env.decay.now))
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            man.UpdateNextMode(sta, env.decay.decay, env.decay.now)
            act.stationId = env.stationId  # only for check

        return act


if __name__ == '__main__':
    ns3Settings = {'raa': 'AiThompsonSampling', 'nWifi': 3, 'standard': '11ac', 'duration': 1}
    mempool_key = 1234 # memory pool key, arbitrary integer large than 1000
    mem_size = 4096 # memory pool size in bytes
    exp = Experiment(mempool_key, mem_size, 'rate-control', '../../')
    exp.reset()

    memblock_key = 2333 # memory block key in the memory pool, arbitrary integer, and need to keep the same in the ns-3 script
    random_stream = 100
    c = AiThompsonSamplingContainer(memblock_key, random_stream)

    pro = exp.run(setting=ns3Settings, show_output=True)
    print("run rate-control", ns3Settings)
    while not c.rl.isFinish():
        with c.rl as data:
            if data == None:
                break
            data.act = c.do(data.env, data.act)
            pass
    
    pro.wait()
    del exp
