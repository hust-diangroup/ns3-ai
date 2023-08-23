# RL-TCP Example

## Introduction

This example applies deep Q-learning algorithm to TCP congestion control for real-time 
changes in the environment of network transmission. By strengthening the learning 
management sliding window and threshold size, the network can get better throughput 
and smaller delay.

### Cmake targets

- `ns3ai_rltcp_gym`: RL-TCP example using Gym interface.
- `ns3ai_rltcp_msg`: RL-TCP example using vector-based message interface.

## Simulation scenario

```
    //Topology in the code
    Left Leafs (Clients)                       Right Leafs (Sinks)
            |            \                    /        |
            |             \    bottleneck    /         |
            |              R0--------------R1          |
            |             /                  \         |
            |   access   /                    \ access |
            N -----------                      --------N
            
```

We construct a dumbbell-type topology simulation scenario in NS3, with only one 
leaf node on the left and right, and two routers R0, R1 on the intermediate link.

### Parameters

|      Parameter       |        Description        | Value  |
|:--------------------:|:-------------------------:|--------|
| bottleneck_bandwidth | bottleneck link bandwidth | 2Mbps  |
|   bottleneck_delay   |  bottleneck link  delay   | 0.01ms |
|   access_bandwidth   |   access link bandwidth   | 10Mbps |
|     access_delay     |     access link delay     | 20ms   |

### Simulation process

- TCP buffer is 4MB, receive and transmit are 2MB respectively; allow sack; DelAckCount 
(Number of packets to wait before sending a TCP ack) is 2.
- The left leaf node sends packets to the right node; initialize a routing table about 
the nodes in the simulation so that the router is aware of all the nodes.
- Output the number of packets received by the right node.

## Running the example

1. [Setup ns3-ai](../../install.md)
2. Run Python script

```shell
pip install -r contrib/ai/examples/rl-tcp/requirements.txt

# Gym interface
cd contrib/ai/examples/rl-tcp/use_gym
python run_rl_tcp.py --use_rl --result --show_log --seed=10

# message interface, vector-based
cd contrib/ai/examples/rl-tcp/use_msg
python run_rl_tcp.py --use_rl --result --show_log --seed=10
```

### Arguments

- `--use_rl`: Use Reinforcement Learning.
- `--result`: Draw figures for the following parameters vs time step:
    - bytesInFlight
    - cWnd
    - segmentsAcked
    - segmentSize
    - ssThresh
- `--show_log`: Output step number, observation received and action sent.
- `--output_dir`: Directory of figures, default to `./result`.
- `--seed`: Python side seed for numpy and torch.

## Results

When `--show_log` is enabled, the Python side output will have the following format:

```text
Step: <current step count>
Send act: [new_cWnd, new_ssThresh]
Recv obs: [ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight]
```

C++ side always prints the number of packets received by the sink. If the seed and duration are the same, the result of 
two interfaces should have no difference.
