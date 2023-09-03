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
- A high-level [Gym interface](model/gym-interface) for using Gymnasium APIs, and a low-level 
[message interface](model/msg-interface) for customizing the shared data.
- Easy to integrate with AI frameworks on Python side.

## Installation

Check out [install.md](./install.md) for how to install and setup ns3-ai.

## Quickstart on ns3-ai

### Demo

To get started on ns3-ai, check out the [A-Plus-B](examples/a-plus-b) example. This example shows how
C++ passes two numbers to Python and their sum is passed back to C++, with the implementation using 
all available interfaces: Gym interface, message interface (struct-based) and message 
interface (vector-based).

### Documentation

Ready to deploy ns3-ai in your own research? Before you code, please go over the tutorials on
[Gym interface](model/gym-interface) and [message interface](model/msg-interface). They provide 
step-by-step guidance on writing C++-Python interfaces, with some useful code snippets.

We also created some **pure C++** examples, which uses C++-based ML frameworks to train 
models. They don't rely on interprocess communication, so there is no overhead in serialization 
and interprocess communication. See [using-pure-cpp](docs/using-pure-cpp.md) for details.

## Examples

Please refer to the README.md in corresponding directories for more information.

### [A-Plus-B](examples/a-plus-b)

This example show how you can use ns3-ai by a very simple case that you transfer `a` and `b` from ns-3 (C++) to Python
and calculate `a + b` in Python to put back the results.

### [Multi-BSS](examples/multi-bss)

In progress...

### [RL-TCP](examples/rl-tcp/)

This example is inspired by [ns3-gym example](https://github.com/tkn-tub/ns3-gym#rl-tcp). We build this example for the
[benchmarking](./docs/benchmarking) and to compare with their module.

### [Rate-Control](examples/rate-control)

This is an example that shows how to develop a new rate control algorithm for the ns-3 Wi-Fi module using ns3-ai.
Available examples are Constant Rate and Thompson Sampling.

### [LTE-CQI](examples/lte-cqi/)

This original work is done based on [5G NR](https://5g-lena.cttc.es/) branch in ns-3. We made some changes to make it
also run in LTE codebase in ns-3 mainline. We didn't reproduce all the experiments on LTE, and the results in our paper
are based on NR work.

## GSoC 2023

- Wiki page: [GSOC2023ns3-ai](https://www.nsnam.org/wiki/GSOC2023ns3-ai)

The new interface utilizes Boost C++ Library and pybind11 to support fast data interaction in C++-Python shared memory.
It is still in development. Two interfaces are introduced before midterm evaluation:

1. Message Interface: Based on Boost.Interprocess shared memory, providing APIs to define 
   message structures and perform low level synchronization. STL container `std::vector` 
   is supported for this interface.
2. Gym Interface: Base on message interface, providing Gym APIs for Python side and environment 
   APIs for C++ side.

## Online Tutorial

This section describes the original design and is not up to date with the newer interface proposed in GSOC 2023.

Join us in this [online recording](https://vimeo.com/566296651) to get better knowledge about ns3-ai! The slides
introduce the ns3-ai model could also be found [here](https://www.nsnam.org/wp-content/uploads/2021/tutorials/ns3-ai-tutorial-June-2021.pdf)!

## Cite Our Work

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
