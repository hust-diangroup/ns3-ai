# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
# Copyright (c) 2019 Huazhong University of Science and Technology, Dian Group
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

import shm_pool
from shm_pool import Init, FreeMemory, GetMemory, RegisterMemory, AcquireMemory, AcquireMemoryCond,\
    AcquireMemoryTarget, AcquireMemoryCondFunc, ReleaseMemory, ReleaseMemoryRB, GetMemoryVersion, IncMemoryVersion
from ctypes import *

READABLE = 0xff
SETABLE = 0


class NS3Var:
    def __init__(self, uid, structType):
        class sType(Structure):
            _pack_ = 1
            _fields_ = [
                ('tagWt', c_ubyte),
                ('tagRd', c_ubyte),
                ('data', structType),
            ]
        self.m_id = uid
        self.m_dataType = sType
        self.m_addr = GetMemory(self.m_id, sizeof(self.m_dataType))
        self.m_obj = self.m_dataType.from_address(self.m_addr)

    def Set(self, data):
        while self.m_obj.tagWt != SETABLE:
            pass
        self.m_obj.data = data
        self.m_obj.tagWt = READABLE

    def Get(self):
        while self.m_obj.tagRd != READABLE:
            pass
        ret = self.m_obj.data
        self.m_obj.tagRd = SETABLE
        return ret

    def GetObj(self):
        return self.m_obj


class NS3BigVar:
    def __init__(self, uid, structType):
        assert issubclass(structType, Structure)
        self.m_id = uid
        self.m_dataType = structType
        self.m_obj = self.m_dataType.from_address(
            RegisterMemory(self.m_id, sizeof(self.m_dataType)))
        self.mod = 2
        self.res = 1

    def GetVersion(self):
        return int(GetMemoryVersion(self.m_id))

    def SetCond(self, mod, res):
        self.mod = mod
        self.res = res

    def __enter__(self):
        # print('enter')
        while self.GetVersion() % self.mod != self.res:
            pass
        AcquireMemory(self.m_id)
        return self.m_obj

    def __exit__(self, Type, value, traceback):
        # print('exit')
        ReleaseMemory(self.m_id)


class EmptyInfo(Structure):
    _pack_ = 1
    _fields_ = [
        ('unused', c_uint8)
    ]


class Ns3AIRL:
    def __init__(self, uid, EnvType, ActType, ExtInfo=EmptyInfo):
        assert issubclass(EnvType, Structure)
        assert issubclass(ActType, Structure)
        assert issubclass(ExtInfo, Structure)

        self.m_id = uid
        self.envType = EnvType
        self.actType = ActType
        self.extInfo = ExtInfo

        class StorageType(Structure):
            _pack_ = 1
            _fields_ = [
                ('env', self.envType),
                ('act', self.actType),
                ('ext', self.extInfo),
                ('isFinish', c_bool)
            ]
        self.type = StorageType
        self.m_obj = self.type.from_address(
            RegisterMemory(self.m_id, sizeof(self.type)))
        self.mod = 2
        self.res = 1

    def GetVersion(self):
        return int(GetMemoryVersion(self.m_id))

    def isFinish(self):
        return self.m_obj.isFinish

    def SetCond(self, mod, res):
        self.mod = mod
        self.res = res

    def Acquire(self):
        while not self.isFinish() and self.GetVersion() % self.mod != self.res:
            pass
        if self.isFinish():
            return None
        AcquireMemory(self.m_id)
        return self.m_obj

    def Release(self):
        ReleaseMemory(self.m_id)

    def ReleaseAndRollback(self):
        ReleaseMemoryRB(self.m_id)

    def __enter__(self):  # ensure Ctrl+C can interrupt the function
        return self.Acquire()

    def __exit__(self, Type, value, traceback):
        self.Release()

class Ns3AIDL:
    def __init__(self, uid, FeatureType, PredictedType, TargetType, ExtInfo=EmptyInfo):
        assert issubclass(FeatureType, Structure)
        assert issubclass(PredictedType, Structure)
        assert issubclass(TargetType, Structure)
        assert issubclass(ExtInfo, Structure)

        self.m_id = uid
        self.featureType = FeatureType
        self.predictedType = PredictedType
        self.targetType = TargetType
        self.extInfo = ExtInfo

        class StorageType(Structure):
            _pack_ = 1
            _fields_ = [
                ('env', self.featureType),
                ('act', self.predictedType),
                ('tar', self.targetType),
                ('ext', self.extInfo),
                ('isFinish', c_bool)
            ]
        self.type = StorageType
        self.m_obj = self.type.from_address(
            RegisterMemory(self.m_id, sizeof(self.type)))
        self.mod = 2
        self.res = 1

    def GetVersion(self):
        return int(GetMemoryVersion(self.m_id))

    def isFinish(self):
        return self.m_obj.isFinish

    def SetCond(self, mod, res):
        self.mod = mod
        self.res = res

    def Acquire(self):
        while not self.isFinish() and self.GetVersion() % self.mod != self.res:
            pass
        if self.isFinish():
            return None
        AcquireMemory(self.m_id)
        return self.m_obj

    def Release(self):
        ReleaseMemory(self.m_id)

    def ReleaseAndRollback(self):
        ReleaseMemoryRB(self.m_id)

    def __enter__(self):  # ensure Ctrl+C can interrupt the function
        return self.Acquire()

    def __exit__(self, Type, value, traceback):
        self.Release()

__all__ = ['Init', 'FreeMemory', 'GetMemory', 'RegisterMemory',
           'AcquireMemory', 'AcquireMemoryCond', 'AcquireMemoryTarget', 'AcquireMemoryCondFunc',
           'ReleaseMemory', 'ReleaseMemoryRB', 'GetMemoryVersion', 'IncMemoryVersion', 'NS3Var', 'NS3BigVar', 'Ns3AIRL']

