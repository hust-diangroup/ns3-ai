# ns3-ai

## Introduction

The [ns–3](https://www.nsnam.org/) simulator is an open-source networking simulation tool implemented by C++ and wildly
used for network research and education. Currently, more and more researchers are willing to apply AI algorithms to
network research. Most AI algorithms are likely to rely on open source frameworks such
as [TensorFlow](https://www.tensorflow.org/) and [PyTorch](https://pytorch.org/). These two parts are developed
independently and extremely hard to merge, so it is more reasonable and convenient to connect these two tasks with data
interaction. Our model provides a high-efficiency solution to enable the data interaction between ns-3 and other python
based AI frameworks.

This module does not provide any AI algorithms or rely on any frameworks but instead is providing a module that
enables AI interconnect, so the AI framework needs to be separately installed. You only need to clone or download this
work, then import the Python modules, you could use this work to exchange data between ns-3 and your AI algorithms.

Inspired by [ns3-gym](https://github.com/tkn-tub/ns3-gym), but using a shared-memory-based approach which is faster and
more flexible.

### Features

- High-performance data interaction module in both C++ and Python side.
- A low-level interface ([msg-interface](model/msg-interface)) for customizing the shared data, and a high-level
  interface ([gym-interface](model/gym-interface)) for using Gymnasium APIs.
- Easy to integrate with AI frameworks on Python side.

## Installation

This installation works on Ubuntu 22.04 and macOS 13.0 or higher.

### Prerequisites

- Boost C++ libraries
    - Ubuntu: `sudo apt install libboost-all-dev`
    - macOS: `brew install boost`
- Protocol buffers
    - Ubuntu:
    ```shell
    # Recommended
    sudo apt install libprotobuf-dev protobuf-compiler
    
    # Try building from source if the above installation causes cmake error
    git clone https://github.com/protocolbuffers/protobuf.git
    cd protobuf
    git submodule update --init --recursive
    cmake -S . -B build -Dprotobuf_BUILD_SHARED_LIBS=ON
    cmake --build build
    sudo cmake --install build
    ```
    - macOS: `brew install protobuf`
- pybind11
    - Ubuntu:
    ```shell
    # Recommended
    sudo apt install pybind11-dev
    
    # Try building from source if the above installation causes cmake error
    git clone https://github.com/pybind/pybind11.git
    cd pybind11
    cmake -S . -B build
    cmake --build build
    sudo cmake --install build
    ```
    - macOS: `brew install pybind11`

### General Setup

1. Clone this repository and configure

```shell
cd contrib
git clone https://github.com/ShenMuyuan/ns3-ai.git
cd ns3-ai
git checkout -b improvements origin/improvements
cd ../../
./ns3 configure --enable-examples
```

2. Build the examples

When building examples, the `ns3-ai` library is built automatically.

```shell
./ns3 build ns3ai_apb_msg ns3ai_apb_gym ns3ai_rltcp_msg ns3ai_rltcp_gym ns3ai_ratecontrol_constant ns3ai_ratecontrol_ts ns3ai_ltecqi
```

3. Setup Gym interface

```shell
cd cd contrib/ns3-ai/model/gym-interface/py
pip install -e .
```

4. Run the examples (optional)

Please check the README.md in corresponding directories for instruction.

## Basic usage

Two interfaces are provided and they have .

### Using the message interface

- C++ side
  TODO

- Python side
  TODO

### Using the gym interface

- C++ side
  TODO

- Python side
  TODO

## Examples

Please check the README.md in corresponding directories for build & run instruction.

### Quick Start on how to use ns3-ai - [a_plus_b](examples/a_plus_b)

This example show how you can use ns3-ai by a very simple case that you transfer the data from ns-3 to python side and
calculate a + b in the python to put back the results.

### [RL-TCP](examples/rl-tcp/)

This example is inspired by [ns3-gym example](https://github.com/tkn-tub/ns3-gym#rl-tcp). We build this example for the
benchmarking and to compare with their module.

### [LTE_CQI](examples/lte_cqi/)

This original work is done based on [5G NR](https://5g-lena.cttc.es/) branch in ns-3. We made some changes to make it
also run in LTE codebase in ns-3 mainline. We didn't reproduce all the experiments on LTE, and the results used in this
document are based on NR work.

### [Rate-Control](examples/rate-control)

This is an example that shows how to develop a new rate control algorithm for the Wi-Fi model in ns-3 using the ns3-ai
model.

- Supported rate control methods

1. Constant Rate Control
2. Thompson Sampling Rate Control

## About the new interface proposed in GSoC 2023

Wiki page: [GSOC2023ns3-ai](https://www.nsnam.org/wiki/GSOC2023ns3-ai)

The new interface utilizes Boost C++ Library and pybind11 to support fast data interaction in C++-Python shared memory.
It is still in development. Two interfaces are introduced before midterm evaluation:

1. [Message Interface](https://github.com/ShenMuyuan/ns3-ai/tree/improvements/model/msg-interface): Based on
   Boost.Interprocess shared memory, providing APIs to define message structures and perform
   low level synchronization. STL container `std::vector` is supported for this interface.
2. [Gym Interface](https://github.com/ShenMuyuan/ns3-ai/tree/improvements/model/gym-interface): Base on message
   interface, providing Gym APIs for Python side and environment APIs for C++ side.

## Online Tutorial

This section describes the original design and is not up to date with the newer interface proposed in GSOC 2023.

Join us in this [online recording](https://vimeo.com/566296651) to get better knowledge about ns3-ai! The slides
introduce the ns3-ai model could also be
found [here](https://www.nsnam.org/wp-content/uploads/2021/tutorials/ns3-ai-tutorial-June-2021.pdf)!

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
