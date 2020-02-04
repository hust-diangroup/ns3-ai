# ns3-ai

## Description
 The [ns–3](https://www.nsnam.org/) simulator is an open-source networking simulation tool implemented by C++ and wildly used for network research and education. Currently, more and more researchers are willing to apply AI algorithms to network research. Most AI algorithms are likely to rely on open source frameworks such as [TensorFlow](https://www.tensorflow.org/) and [PyTorch](https://pytorch.org/). These two parts are developed independently and extremely hard to merge, so it is more reasonable and convenient to connect these two tasks with data interaction. Our model provides a high-efficiency solution to enable the data interaction between ns-3 and other python based AI frameworks.

 Inspired by [ns3-gym](https://github.com/tkn-tub/ns3-gym), but using a different approach which is faster and more flexible.

### Features
- High-performance data interaction module (using shared memory). 
- Provide a high-level interface for different AI algorithms.
- Easy to integrate with other AI frameworks.


## Installation
### 1. Install this module in ns-3
#### Get ns-3:  
This module needs to be built within ns-3, so you need to get a ns-3-dev or other ns-3 codes first.

Check [ns-3 installation wiki](https://www.nsnam.org/wiki/Installation) for detailed instructions.

#### Add this module
```
cd $YOUR_NS3_CODE/contrib
git clone https://github.com/hust-diangroup/ns3-ai.git
```

#### Rebuild ns-3
```
./waf configure
./waf
```

### 2. Add Python interface

#### Install
Python3 is used and tested.

```
cd $YOUR_NS3_CODE/contrib/ns3-ai/py_interface

python setup.py install --user
```

#### Baisc usage
```
import py_interface
py_interface.Init(1234, 4096) # key poolSize
v = NS3BigVar(233, c_int*10)
with v as o:
    for i in range(10):
        o[i] = c_int(i)
    print(*o)
py_interface.FreeMemory()
```
## Shared Memory Pool
The ns3-ai module interconnects the ns-3 and AI frameworks by transferring data through the shared memory pool. The memory can be accessed by both sides and controlled mainly in ns-3. The shared memory pool is defined in `ns3-ai/model/memory-pool.h`.


## Examples
### [RL-TCP](https://github.com/hust-diangroup/ns3-ai/blob/master/example/rl-tcp/RL-TCP-en.md)
This example is inspired by [ns3-gym example](https://github.com/tkn-tub/ns3-gym#rl-tcp). We bulid this example for the benchmarking and to compare with their module.

#### Build and Run
Run ns-3 example:
```
cp -r contrib/ns3-ai/example/rl-tcp scratch/

./waf --run "rl-tcp"
```
Run Python code:
```
cd src/ns3-ai/example/rl-tcp/

python testtcp.py
```
**NOTE: Currently the RL test in python script is not fully enabled, coming soon.**

### [LTE_CQI](https://github.com/hust-diangroup/ns3-ai/blob/master/example/lte_cqi/Lte_CQI.md)
This original work is done based on [5G NR](https://5g-lena.cttc.es/) branch in ns-3. We made some changes to make it also run in LTE codebase in ns-3 mainline. We didn't reproduce all the experiments on LTE, and the results used in this document are based on NR work.

#### Build and Run
Apply the lte patch (in your ns3 dir)
```
cp src/ns3-ai/example/lte_cqi/lte_cqi_predict.patch .

git apply --stat lte_cqi_predict.patch

git apply --check lte_cqi_predict.patch

git am -s < lte_cqi_predict.patch
```

Run ns-3 example:
```
cp -r src/ns3-ai/example/lte_cqi scratch/

./waf --run "lte_cqi"
```
Run Python code:
```
cd scratch/lte_cqi/

python run_online.py 
```    
If you want to test the LSTM, you can run another python script but you may need to install [PyTorch](https://pytorch.org/) environment first. 
```
cd scratch/lte_cqi/

python run_online_lstm.py
```    
**NOTE: If the program does not exit normally, you need to run freeshm.sh to release the shared memory manually.**

