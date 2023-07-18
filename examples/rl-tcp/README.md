# RL-TCP

## Objective

- Apply RL algorithm to TCP congestion control for real-time changes in the environment of network transmission. By
  strengthening the learning management sliding window and threshold size, the network can get better throughput and
  smaller delay.

- Test the data interaction functions and the performance of ns3-ai module.

## TCP simulation in NS3

### Simulation scenario

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

- Test the TcpNewReno congestion control algorithm.
- Constructing a dumbbell-type topology simulation scenario in NS3, with only one leaf node on the left and right, and
  two routers R0, R1 on the intermediate link.

### Test parameters：

|      Parameter       |        Description        | Value  |
|:--------------------:|:-------------------------:|--------|
| bottleneck_bandwidth | bottleneck link bandwidth | 2Mbps  |
|   bottleneck_delay   |  bottleneck link  delay   | 0.01ms |
|   access_bandwidth   |   access link bandwidth   | 10Mbps |
|     access_delay     |     access link delay     | 20ms   |

### Simulation process：

- TCP buffer is 4MB, receive and transmit are 2MB respectively; allow sack; DelAckCount (Number of packets to wait
  before sending a TCP ack) is 2.

- The left leaf node sends packets to the right node; initialize a routing table about the nodes in the simulation so
  that the router is aware of all the nodes.

- Output the number of packets received by the right node.

## RL Algorithm

* Apply Deep Q-learning algorithm to TCP congestion control.
* In RL Algorithm, output actions and reward to determine the performance of model.

## Interaction between RL and ns-3

- In ns-3, put data such as segmentsAcked into shared memory for use by the RL algorithm.

- Python trains based on the read data to get the size of the new congestion window and threshold; put it into shared
  memory, and update it on NS3.

## Run RL-TCP example (msg interface)

1. [General setup](https://github.com/ShenMuyuan/ns3-ai/tree/improvements#general-setup)

2. Run the example

- You must run Python side first, because Python script is the shared memory creator.

```shell
cd contrib/ns3-ai/examples/rl-tcp/use_msg
python run_rl_tcp.py
```

Arguments:

  - `--use_rl`: Use RL rather than traditional TCP algorithm
  - `--result`: Draw figures for the following parameters vs time step:
      - bytesInFlight
      - cWnd
      - segmentsAcked
      - segmentSize
      - ssThresh
  - `--output_dir`: Directory of figures, default to `./result`.

- When you see the message `Created message interface, waiting for C++ side to send initial environment...`, Open
  another terminal and run C++ side.

```shell
./ns3 run ns3ai_rltcp_msg
```

By default, the simulation time is 1000s and time step is 10ms.

## Run RL-TCP example (Gym interface)

1. [General setup](https://github.com/ShenMuyuan/ns3-ai/tree/improvements#general-setup)

2. Run the example

- You must run Python side first, because Python script is the shared memory creator.

```shell
cd contrib/ns3-ai/examples/rl-tcp/use_gym
python test_tcp.py
```

- When you see the message `Created message interface, waiting for C++ side to send initial environment...`, Open
  another terminal and run C++ side.

```shell
./ns3 run ns3ai_rltcp_gym
```

# Output

TODO
