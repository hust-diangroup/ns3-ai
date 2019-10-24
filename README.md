# ns-3-AI-interface
Enable the interaction between ns-3 and popular frameworks using Python, which mean you can train and test your ml algorithms in ns-3 without changing any frameworks you are using now! 

## Use Shared Memory in NS-3
### 1. Copy module to src/
```
cp -r shared-memory/ /path/to/ns3/src/
```
### 2. Rebuild NS-3
```
./waf configure
./waf
```

## Run Example NS-3 Code
```
copy src/shared-memory/example/test-shm.cc scratch/
./waf --run scratch/test-shm
```

## Run Python Code
```python
python test.py
```

## NOTICE: C/C++ code and Python code need to run simultaneously
