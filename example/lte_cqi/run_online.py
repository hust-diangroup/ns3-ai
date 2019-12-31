from py_interface import *
from ctypes import *
Init(1234, 4096)

MAX_RBG_NUM = 32


class CqiFeature(Structure):
    _pack_ = 1
    _fields_ = [
        ('wbCqi', c_uint8),
        ('rbgNum', c_uint8),
        ('nLayers', c_uint8),
        ('sbCqi', (c_uint8*MAX_RBG_NUM)*2)
    ]


class CqiPredicted(Structure):
    _pack_ = 1
    _fields_ = [
        ('new_wbCqi', c_uint8),
        ('new_sbCqi', (c_uint8*MAX_RBG_NUM)*2)
    ]

class CqiTarget(Structure):
    _pack_ = 1
    _fields_ = [
        ('target', c_uint8)
    ]

dl = Ns3AIDL(1357, CqiFeature, CqiPredicted, CqiTarget)
# dl.SetCond(2, 1)
try:
    while True:
        with dl as data:
            if data == None:
                break
            # print('data.env.wbCqi', data.env.wbCqi)
            # Deep Learning code there
            data.act.new_wbCqi = data.env.wbCqi
            data.act.new_sbCqi = data.env.sbCqi
except KeyboardInterrupt:
    print('Ctrl C')
finally:
    FreeMemory()
