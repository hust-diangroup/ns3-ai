# Get CPU cycle diff
## Install
```bash
python setup.py install --user
```
## Usage
```python
import py_cycle
print(py_cycle.getCycle())# Get current CPU cycle

# If cycleDiff(id) has not be called, the function will return None, else return CPU cycle between nearest cycleDiff(id) called. 
py_cycle.cycleDiff(1)
for i in range(1000000):
    pass
print(py_cycle.cycleDiff(1))

py_cycle.cycleDiff(2)
for i in range(100):
    for j in range(10000):
        pass
    print(py_cycle.cycleDiff(2))
print(py_cycle.getStatistic(2))# return statistic info. tuple(count,total,average,min,max)
```
