# ns3-ai
 The [ns–3](https://www.nsnam.org/) simulator is an open-source networking simulation tool implemented by C++ and wildly used for network research and education. Currently, more and more researchers are willing to apply AI algorithms to network research. Most AI algorithms are likely to rely on open source frameworks such as [TensorFlow](https://www.tensorflow.org/) and [PyTorch](https://pytorch.org/). These two parts are developed independently and extremely hard to merge, so it is more reasonable and convenient to connect these two tasks with data interaction. Our model provides a high-efficiency solution to enable the data interaction between ns-3 and other python based AI frameworks.

 Inspired by [ns3-gym](https://github.com/tkn-tub/ns3-gym), but using a different approach which is faster and more flexible.

### Features
- High-performance data interaction module (using shared memory). 
- Provide a high-level interface for different AI algorithms.
- Easy to integrate with other AI frameworks.


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
