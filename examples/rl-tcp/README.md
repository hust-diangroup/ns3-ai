# RL-TCP Example

## Introduction

This example applies Q-learning algorithms to TCP congestion control for real-time
changes in the environment of network transmission. By optimizing cWnd (contention 
window) and ssThresh (slow start threshold), the network can get better throughput
and smaller delay.

### Cmake targets

- `ns3ai_rltcp_gym`: RL-TCP example using Gym interface.
- `ns3ai_rltcp_msg`: RL-TCP example using vector-based message interface.

## Algorithms

### RL: Q-learning and Deep Q-learning

Q-learning is based on estimating the values of state-action pairs
in a Markov decision process, by iteratively updating an action-value
function. In this example's implementation, the Q-table is updated
each time ns-3 interacts with Python side, and the agent chooses
cWnd and ssThresh according to epsilon-greedy algorithm.

Deep Q-learning, on the other hand, is a variant of Q-learning that
utilizes a deep neural network to approximate the Q-values. Here the
DQN is also updated at every C++-Python interaction.

### Non-RL: TcpNewReno

TcpNewReno is a TCP layer congestion control algorithm which employs
a "fast recovery" mechanism, which allows it to detect lost packets
more quickly compared to the standard Reno algorithm. In this example,
if RL algorithm is not selected, the algorithm will be TcpNewReno.

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

### Gym interface

1. [Setup ns3-ai](../../docs/install.md)
2. Build C++ executable & Python bindings

```shell
cd YOUR_NS3_DIRECTORY
./ns3 build ns3ai_rltcp_gym
```

3. Run Python script

The following code selects deep Q-learning to TCP congestion control.

```shell
pip install -r contrib/ai/examples/rl-tcp/requirements.txt
cd contrib/ai/examples/rl-tcp/use-gym
python run_rl_tcp.py --use_rl --result --show_log --seed=10
```

### Message interface (vector-based)

1. [Setup ns3-ai](../../docs/install.md)
2. Build C++ executable & Python bindings

```shell
cd YOUR_NS3_DIRECTORY
./ns3 build ns3ai_rltcp_msg
```

3. Run Python script

The following code selects deep Q-learning to TCP congestion control.

```shell
pip install -r contrib/ai/examples/rl-tcp/requirements.txt
cd contrib/ai/examples/rl-tcp/use-msg
python run_rl_tcp.py --use_rl --rl_algo=DeepQ --result --show_log --seed=10
```

### Arguments

- `--use_rl`: Use Reinforcement Learning. If not specified, program will use `TcpNewReno`.
- `--rl_algo`: RL algorithm to apply, can be `DeepQ` for deep Q-learning or `Q` for Q-learning. Defaults to `DeepQ`.
- `--result`: Draw figures for the following parameters vs time step:
    - bytesInFlight
    - cWnd
    - segmentsAcked
    - segmentSize
    - ssThresh
- `--show_log`: Output step number, observation received and action sent.
- `--output_dir`: Directory of figures relative from `YOUR_NS3_DIRECTORY`, defaults to `./rl_tcp_results`.
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
