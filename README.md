# ns-3-AI-interface
Enable the interaction between ns-3 and popular frameworks using Python, which mean you can train and test your ml algorithms in ns-3 without changing any frameworks you are using now! 

## Get ns-3:  
This module needs to be built within ns-3, so you need to get a ns-3-dev or other ns-3 codes first.

## Add this module
```
cd $YOURNS3CODE/src
git clone https://github.com/hust-diangroup/ns-3-AI-interface.git
```

## Use Shared Memory in NS-3

### 1. Rebuild NS-3
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
