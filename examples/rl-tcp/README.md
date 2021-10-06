# RL-TCP

## Objective

- Apply RL algorithm to TCP congestion control, for real-time changes in the environment of network transmission, by strengthening the learning management sliding window and threshold size, the network can get better throughputs and smaller delay.

- Test the data interaction functions and the performance of ns-3-AI module.

## TCP simulation in NS3


### Simulation scenario

```
    //Topology in the code
    Right Leafs (Clients)                      Left Leafs (Sinks)
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

- TCP buffer is 4MB, receive and transmit are 2MB respectively; allow sack; DelAckCount （Number of packets to wait before sending a TCP ack） is 2.

- The left leaf node sends  packets to the right node; initialize a routing table about the nodes in the simulation so that the router is aware of all the nodes.

-  Output the number of packets received by the right node.





## RL Algorithm

* Apply Deep Q-learning algorithm to TCP congestion control
* In RL Algorithm, output actions and reward determine the performance of model



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
m_new_cWnd = act->new_cWnd;//Read trained new contrl widnow
m_new_ssThresh = act->new_ssThresh;
```



- Python trains based on the read data to get the size of the new control window and threshold; put it into shared memory, and update it on NS3.

python  side：

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

python3 run_tcp_rl.py --use_rl --result
```

