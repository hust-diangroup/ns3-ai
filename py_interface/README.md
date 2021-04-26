# NS-3 AI Python interface
# Install
```bash
pip3 install /path/to/py_interface
```
# Usage
```python
import py_interface
py_interface.Init(1234, 4096) # key poolSize
v = ShmBigVar(233, c_int*10)
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
exp = Experiment(1234, 4096, 'multi-run', '../../')
for i in range(2):
    exp.reset()
    rl = Ns3AIRL(2333, Env, Act)
    pro = exp.run()
    while not rl.isFinish():
        with rl as data:
            if data == None:
                break
            data.act.c = data.env.a+data.env.b
    pro.wait()
del exp
```
