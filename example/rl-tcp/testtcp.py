import py_interface
from ctypes import *


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
    ]


class TcpRlAct(Structure):
    _pack_ = 1
    _fields_ = [
        ('new_ssThresh', c_uint32),
        ('new_cWnd', c_uint32)
    ]


py_interface.Init(1234, 4096)

var = py_interface.Ns3AIRL(1234, TcpRlEnv, TcpRlAct)

# var = py_interface.ShmBigVar(1234, TcpRl)
while not var.isFinish():
    # while var.GetVersion() % 2 != 1:
    #     pass
    # with var as data:
    #     # print(data.env.ssThresh, data.env.cWnd)
    with var as data:
        if data == None:
            break
        print(var.GetVersion())
        ssThresh = data.env.ssThresh
        cWnd = data.env.cWnd
        segmentsAcked = data.env.segmentsAcked
        segmentSize = data.env.segmentSize
        bytesInFlight = data.env.bytesInFlight
        print(ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight)
        new_cWnd = 1
        new_ssThresh = 1

        # IncreaseWindow
        if (cWnd < ssThresh):
            # slow start
            if (segmentsAcked >= 1):
                new_cWnd = cWnd + segmentSize

        if (cWnd >= ssThresh):
            # congestion avoidance
            if (segmentsAcked > 0):
                adder = 1.0 * (segmentSize * segmentSize) / cWnd
                adder = int(max(1.0, adder))
                new_cWnd = cWnd + adder

        # GetSsThresh
        new_ssThresh = int(max(2 * segmentSize, bytesInFlight / 2))
        data.act.new_cWnd = new_cWnd
        data.act.new_ssThresh = new_ssThresh

py_interface.FreeMemory()
