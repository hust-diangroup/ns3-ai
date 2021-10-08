# NS-3 AI Python interface
# Install
```bash
pip3 install /path/to/py_interface
```
# Usage
```python
import py_interface
mempool_key = 1234                                          # memory pool key, arbitrary integer large than 1000
mem_size = 4096                                             # memory pool size in bytes
memblock_key = 2333                                         # memory block key, need to keep the same in the ns-3 script
py_interface.Init(mempool_key, mem_size) # key poolSize
v = ShmBigVar(memblock_key, c_int*10)
with v as o:
    for i in range(10):
        o[i] = c_int(i)
    print(*o)
py_interface.FreeMemory()
```

# Work with NS-3(ns3-ai/example/multi-run)
```python
from py_interface import *
from ctypes import *

class Env(Structure):
    _pack_ = 1
    _fields_ = [
        ('a', c_int),
        ('b', c_int)
    ]
class Act(Structure):
    _pack_ = 1
    _fields_ = [
        ('c', c_int)
    ]
mempool_key = 1234                                          # memory pool key, arbitrary integer large than 1000
mem_size = 4096                                             # memory pool size in bytes
memblock_key = 2333                                         # memory block key, need to keep the same in the ns-3 script
exp = Experiment(mempool_key, mem_size, 'multi-run', '../../')
for i in range(2):
    exp.reset()
    rl = Ns3AIRL(memblock_key, Env, Act)
    pro = exp.run()
    while not rl.isFinish():
        with rl as data:
            if data == None:
                break
            data.act.c = data.env.a+data.env.b
    pro.wait()
del exp
```
