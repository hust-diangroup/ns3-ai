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
#         Xiaojun Guo <guoxj@hust.edu.cn>
#         Hao Yin <haoyin@uw.edu>

# should run with ns3 code (cd $YOUR_NS3_CODE; ./waf --run "lte_cqi") simultaneously

from py_interface import *
from ctypes import *
mempool_key = 1234          # memory pool key, arbitrary integer large than 1000
mem_size = 4096             # memory pool size in bytes
Init(mempool_key, mem_size) # Init shared memory pool

MAX_RBG_NUM = 32

# The feature of DL training (in this example, feature of cqi)
# shared between ns-3 and python with the same shared memory
# using the ns3-ai model.
class CqiFeature(Structure):
    _pack_ = 1
    _fields_ = [
        ('wbCqi', c_uint8),                 # wide band cqi
        ('rbgNum', c_uint8),                # resource block group number
        ('nLayers', c_uint8),               # number of layers
        ('sbCqi', (c_uint8*MAX_RBG_NUM)*2)  # sub band cqi
    ]

# The prediction of DL training (in this example, prediction of cqi)
# calculated by python and put back to ns-3 with the shared memory.
class CqiPredicted(Structure):
    _pack_ = 1
    _fields_ = [
        ('new_wbCqi', c_uint8),
        ('new_sbCqi', (c_uint8*MAX_RBG_NUM)*2)
    ]

# The target of DL training (in this example, target of cqi)
class CqiTarget(Structure):
    _pack_ = 1
    _fields_ = [
        ('target', c_uint8)
    ]

memblock_key = 1357         # memory block key, need to keep the same in the ns-3 script
dl = Ns3AIDL(memblock_key, CqiFeature, CqiPredicted, CqiTarget)     # Link the shared memory block with ns-3 script
# dl.SetCond(2, 1)
try:
    while True:
        with dl as data:
            if data == None:
                break
            # print('data.feat.wbCqi', data.feat.wbCqi)
            # Deep Learning code there
            data.pred.new_wbCqi = data.feat.wbCqi
            data.pred.new_sbCqi = data.feat.sbCqi
except KeyboardInterrupt:
    print('Ctrl C')
finally:
    FreeMemory()            # Free shared memory pool
