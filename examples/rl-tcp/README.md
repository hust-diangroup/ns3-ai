# RL-TCP

## Objective

- Apply RL algorithm to TCP congestion control for real-time changes in the environment of network transmission. By strengthening the learning management sliding window and threshold size, the network can get better throughput and smaller delay.

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
- Constructing a dumbbell-type topology simulation scenario in NS3, with only one leaf node on the left and right, and two routers R0, R1 on the intermediate link.

### Test parameters：

|      Parameter       |        Description        | Value  |
| :------------------: | :-----------------------: | ------ |
| bottleneck_bandwidth | bottleneck link bandwidth | 2Mbps  |
|   bottleneck_delay   |  bottleneck link  delay   | 0.01ms |
|   access_bandwidth   |   access link bandwidth   | 10Mbps |
|     access_delay     |     access link delay     | 20ms   |

### Simulation process：

- TCP buffer is 4MB, receive and transmit are 2MB respectively; allow sack; DelAckCount (Number of packets to wait before sending a TCP ack) is 2.

- The left leaf node sends  packets to the right node; initialize a routing table about the nodes in the simulation so that the router is aware of all the nodes.

-  Output the number of packets received by the right node.





## RL Algorithm

* Apply Deep Q-learning algorithm to TCP congestion control.
* In RL Algorithm, output actions and reward to determine the performance of model.



## Interaction between RL and ns-3


- Set up the environment

```
ns-3:
env = Create<TcpTimeStepShmEnv> (1234);//1234 shared-memory key


python：
py_interface.Init(1234, 4096)#pool size = 4096
var = py_interface.ShmBigVar(1234, TcpRl)
```



- In ns-3, put data such as segmentsAcked into shared memory for use by the RL algorithm.

ns-3 side：

```
auto env = EnvSetter ();

env->ssThresh = m_tcb->m_ssThresh;
env->cWnd = m_tcb->m_cWnd;
env->segmentSize = m_tcb->m_segmentSize;//Length of data segment sent by TCP at one time
env->bytesInFlight = bytesInFlightSum;//Bytes in flight is the amount of data that has been sent but not yet acknowledged
env->segmentsAcked = segmentsAckedSum;


auto act = ActionGetter ();
m_new_cWnd = act->new_cWnd;//Read trained new congestion window
m_new_ssThresh = act->new_ssThresh;
```



- Python trains based on the read data to get the size of the new congestion window and threshold; put it into shared memory, and update it on NS3.

python side：

```
 ssThresh = data.env.ssThresh
 cWnd = data.env.cWnd
 segmentsAcked = data.env.segmentsAcked
 segmentSize = data.env.segmentSize
 bytesInFlight = data.env.bytesInFlight
 
 
 data.act.new_cWnd = new_cWnd
 data.act.new_ssThresh = new_ssThresh
```



## Build and Run

Run code:
```
cd contrib/ns3-ai/example/rl-tcp/

python3 run_rl_tcp.py --use_rl --result
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

By default, the simulation time is 1000s, which is configured in `rl-tcp.cc` using CommandLine value `duration`. Time step
interval is 10ms, which is configured in `tcp-rl-env.h` using `m_timeStep`.

## Output

At each time step, states and actions and corresponding time are shown.

For example:
```c++
At 163ms:
        state -- ssThresh=4294967295 cWnd=3400 segmentSize=340 segmentAcked=1 bytesInFlightSum=3060
        action -- new_cWnd=3740 new_ssThresh=680
At 173ms:
        state -- ssThresh=4294967295 cWnd=3740 segmentSize=340 segmentAcked=6 bytesInFlightSum=9180
        action -- new_cWnd=4080 new_ssThresh=4590
At 183ms:
        state -- ssThresh=4294967295 cWnd=4080 segmentSize=340 segmentAcked=2 bytesInFlightSum=3060
        action -- new_cWnd=4108 new_ssThresh=680
.
.
.

```

Output can be turned off by modifying`run_rl_tcp.py`:

Change
```c++
exp.run(show_output=1)
```

to
```c++
exp.run(show_output=0)
```
