# ns3-ai
## Online Tutorial:
Join us in this [online recording](https://vimeo.com/566296651) to get better knowledge about ns3-ai! The slides introduce the ns3-ai model could also be found [here](https://www.nsnam.org/wp-content/uploads/2021/tutorials/ns3-ai-tutorial-June-2021.pdf)! 
## Description
 The [ns–3](https://www.nsnam.org/) simulator is an open-source networking simulation tool implemented by C++ and wildly used for network research and education. Currently, more and more researchers are willing to apply AI algorithms to network research. Most AI algorithms are likely to rely on open source frameworks such as [TensorFlow](https://www.tensorflow.org/) and [PyTorch](https://pytorch.org/). These two parts are developed independently and extremely hard to merge, so it is more reasonable and convenient to connect these two tasks with data interaction. Our model provides a high-efficiency solution to enable the data interaction between ns-3 and other python based AI frameworks.

 This module does not provide any AI algorithms or rely on any frameworks but instead is providing a Python module that enables AI interconnect, so the AI framework needs to be separately installed. You only need to clone or download this work, then import the Python modules, you could use this work to exchange data between ns-3 and your AI algorithms.


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
```Shell
cd $YOUR_NS3_CODE/contrib
git clone https://github.com/hust-diangroup/ns3-ai.git
```

#### Rebuild ns-3
```Shell
./waf configure
./waf
```

### 2. Add Python interface

#### Install
Python3 is used and tested.

```Shell
cd $YOUR_NS3_CODE/contrib/ns3-ai/py_interface

pip3 install . --user
```

#### Baisc usage
``` Python
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
## Shared Memory Pool
The ns3-ai module interconnects the ns-3 and AI frameworks by transferring data through the shared memory pool. The memory can be accessed by both sides and controlled mainly in ns-3. The shared memory pool is defined in `ns3-ai/model/memory-pool.h`.  
The `CtrlInfoBlock` is the control block of the all shared memory pool, the `SharedMemoryCtrl` is the control block of each shared memory, and the `SharedMemoryLockable` is the actual shared memory used for data exchange. In each memory block, we use version and nextVersion as the lock indicator. The synchronization for reading/writing locks and the events update are accomplished by the lock indicator. For every process that wants to access or modify the data, it will compare the `version` variable and the `nextVersion` variable. If they are the same, it means that the memory is reachable. Then it will add one to the next version atomically to lock the memory and also add one to the version after its operation to the memory to unlock the memory. Besides the version of the memory acts as the signal to tell different processes the current state of the memory block, which provides different methods to synchronize.
```
|SharedMemoryBlock1|
|SharedMemoryBlock2|
|SharedMemoryBlock3|
...
...
...
|ControlMemoryBlock3|
|ControlMemoryBlock2|
|ControlMemoryBlock1|
|MemoryPoolContrlBlk|
```



## Examples
### Quick Statrt on how to us ns3-ai - [a_plus_b](https://github.com/hust-diangroup/ns3-ai/tree/master/examples/a_plus_b)
This example show how you can use ns3-ai by a very simple case that you transfer the data from ns-3 to python side and calculate a + b in the python to put back the results. Please check the README in it for more details.

### [RL-TCP](https://github.com/hust-diangroup/ns3-ai/blob/master/examples/rl-tcp/)
This example is inspired by [ns3-gym example](https://github.com/tkn-tub/ns3-gym#rl-tcp). We bulid this example for the benchmarking and to compare with their module.

#### Build and Run
Run ns-3 example:
```
cp -r contrib/ns3-ai/example/rl-tcp scratch/
python3 run_tcp_rl.py --use_rl --result
```

### [LTE_CQI](https://github.com/hust-diangroup/ns3-ai/blob/master/examples/lte_cqi/)
This original work is done based on [5G NR](https://5g-lena.cttc.es/) branch in ns-3. We made some changes to make it also run in LTE codebase in ns-3 mainline. We didn't reproduce all the experiments on LTE, and the results used in this document are based on NR work.

#### Build and Run

Run ns-3 example:
If you want to test the LSTM, you can run another python script but you may need to install [TensorFlow](https://www.tensorflow.org/) environment first. 
```Shell
cd scratch/lte_cqi/

python3 run_online_lstm.py 1
```    
**NOTE: If the program does not exit normally, you need to run freeshm.sh to release the shared memory manually.**

### [Rate-Control](https://github.com/hust-diangroup/ns3-ai/tree/master/examples/rate-control)
This is an example that shows how to develop a new rate control algorithm for the Wi-Fi model in ns-3 using the ns3-ai model.
#### Usage

Copy this example to scratch:

```shell
cp -r contrib/ns3-ai/example/rate-control scratch/
cd scratch/rate-control
```

##### 1. Constant Rate Control

```shell
python3 ai_constant_rate.py
```

##### 2. Thompson Sampling Rate Control

```shell
python3 ai_thompson_sampling.py
```

## Cite our work
Please use the following bibtex:
```
@inproceedings{10.1145/3389400.3389404,
author = {Yin, Hao and Liu, Pengyu and Liu, Keshu and Cao, Liu and Zhang, Lytianyang and Gao, Yayu and Hei, Xiaojun},
title = {Ns3-Ai: Fostering Artificial Intelligence Algorithms for Networking Research},
year = {2020},
isbn = {9781450375375},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
url = {https://doi.org/10.1145/3389400.3389404},
doi = {10.1145/3389400.3389404},
booktitle = {Proceedings of the 2020 Workshop on Ns-3},
pages = {57–64},
numpages = {8},
keywords = {AI, network simulation, ns-3},
location = {Gaithersburg, MD, USA},
series = {WNS3 2020}
}
  
```
