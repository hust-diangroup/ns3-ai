# NS-3 AI Python interface
# Install
```bash
python setup.py install --user
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