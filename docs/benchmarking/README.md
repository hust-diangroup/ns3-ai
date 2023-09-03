# ns3-ai Benchmarking

## Gym interface vs. ns3-gym

This benchmark is based on the [RL-TCP](../../examples/rl-tcp) example, 
in which we record the CPU cycle count during C++ to Python and Python to 
C++ data transmissions, and compare the mean and standard deviation of cycles.

In this benchmark, we seed both programs with the same numpy and torch 
seeds, and `SeedManager::SetRun` is set to default (0).

### Code

- Gym interface is based on [midterm_benchmarking](https://github.com/ShenMuyuan/ns3-ai/tree/midterm_benchmarking) 
branch of ns3-ai.
- ns3-gym is based on a [forked branch](https://github.com/ShenMuyuan/ns3-gym) 
of ns3-gym, in which we modified the algorithm to yield the same result 
with ns3-ai.

The results are collected from console output and stored in [a .m file](./gym-interface-data.m) 
to draw figures with Matlab.

### Results

Results show that in both directions, the transmission time of ns3-ai's Gym 
interface is more than 15 times shorter than that of ns3-gym.

<p align="center">
    <img src="./gym-interface-figure-cpp2py.png" alt="gym cpp2py" width="600"/>
</p>

<p align="center">
    <img src="./gym-interface-figure-py2cpp.png" alt="gym py2cpp" width="600"/>
</p>

## Vector-based vs. struct-based

TODO

## Pure C++ vs. C++-Python interface

The benchmark is based on the [pure C++ (libtorch)](../../examples/rl-tcp/pure_cpp) and 
[message interface (PyTorch)](../../examples/rl-tcp/use_msg) version of RL-TCP example. 
We compare the processing time (i.e. transmission time + DRL algorithm time 
for message interface, DRL algorithm time for pure C++) for the two interfaces, 
including the mean and the standard deviation. The difference can be considered 
as the CPU cycles saved, due to:
1. The removal of interprocess communication, which saves thread waiting and 
data serialization time.
2. The native C++ implementation of DRL algorithm with `libtorch`, which can 
be potentially faster than PyTorch (a Python wrapper of the C++ core).

In this benchmark, we seed two programs with same `SeedManager::SetRun`.

**Note: the simulation results of message interface and pure C++ implementations 
may be different, despite having same parameters. This is due to different 
random seeding (the pure C++ example doesn't provide seeding equivalent to numpy's 
seeding).**

### Code

Both sides of the benchmarking code can be found at [benchmark_purecpp branch](https://github.com/ShenMuyuan/ns3-ai/tree/benchmark_purecpp). 

The results are collected from console output and stored in [a .m file](./pure-cpp-data.m)
to draw figures with Matlab.

### Results

Results show that the processing time of pure C++ implementation 
is more than twice shorter than that of message interface implementation.

<p align="center">
    <img src="./pure-cpp-figure.png" alt="processing" width="600"/>
</p>


