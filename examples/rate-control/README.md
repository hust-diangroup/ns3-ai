# Rate Control Example

There are existing models of constant rate and Thompson sampling algorithms in Wi-Fi module. Here we reimplement 
them in Python to show how to develop a new rate control algorithm for the Wi-Fi module using ns3-ai.

## About Thompson Sampling Example

- In the algorithm, each action has parameters alpha and beta attached to it (and independent with other actions), used to estimate probability of success. 
Our goal is to find the action with the highest estimated probability of success.
- Unlike greedy algorithm which estimates probability using counters (success / total), TS algorithm
estimates by sampling from posterior beta distribution (defined with alpha and beta).
- By choosing the highest estimated probability, TS probes potentially good actions, avoids 
unuseful actions, and finally converge at the optimal one.

### Applying TS to MCS selection & policy improvement

- Each MCS is an action, which has two results: successful or failed transmission.
- When deciding a new MCS, estimate every MCS its probability of transmission success.
- Select the MCS with the highest estimated probability.
- After transmission, use the result to update parameters alpha and beta.

### Interaction between ns-3 and ns3-ai

In general, C++ code stores the environment in shared memory (for C++ side, write only,
for Python side, read only). Python code updates action to another
part of shared memory (for Python side, write only, for C++ side, read only).

- C++ side (ns-3)

The type of environment indicates different events (e.g. 
initialization, tx start, tx success, tx failure) that occurs 
in ns-3 simulation.

- Python side (ns3-ai)

Python code handles environment in shared memory according to the type number.
For example, when a transmission is over (data failed, type 0x05, or data OK, type 0x06), 
statistics will be updated and an optimal MCS is selected for next transmission.

## Build and Run

### Clone the examples

```shell
cd contrib
git clone https://github.com/ShenMuyuan/ns3-ai.git
cd ns3-ai
git checkout -b improvements origin/improvements
cd ../../
```

### Running Constant Rate Control

In one terminal:

```shell
./ns3 clean
./ns3 configure --enable-examples
./ns3 build ns3ai_ratecontrol_constant
cd contrib/ns3-ai/examples/rate-control/constant
python ai_constant_rate.py
```

In another terminal:

```shell
./ns3 run ns3ai_ratecontrol_constant
```

### Running Thompson Sampling Rate Control

In one terminal:

```shell
./ns3 clean
./ns3 configure --enable-examples
./ns3 build ns3ai_ratecontrol_ts
cd contrib/ns3-ai/examples/rate-control/thompson-sampling
python ai_thompson_sampling.py
```

In another terminal:

```shell
./ns3 run ns3ai_ratecontrol_ts -- --raa=AiThompsonSampling --nWifi=3 --standard=11ac --duration=5
```

## Output

For Constant Rate example, you will see:

```
nWifi: 3, RAA Algorithm: MinstrelHt, duration: 1
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 12.6419ms, Throughput: 3.02063Mbps
Delay: 16.1259ms, Throughput: 16.4877Mbps
Delay: 25.5676ms, Throughput: 38.3762Mbps
Delay: 30.5037ms, Throughput: 39.097Mbps
Delay: 28.2345ms, Throughput: 37.4466Mbps
Delay: 26.58ms, Throughput: 49.5081Mbps
Delay: 25.0521ms, Throughput: 44.7327Mbps
Delay: 29.7663ms, Throughput: 46.7398Mbps
Delay: 28.1572ms, Throughput: 41.8506Mbps
Delay: 34.3698ms, Throughput: 41.4523Mbps
Delay: 25.903ms, Throughput: 15.2505Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Delay: 0ms, Throughput: 0Mbps
Total Bytes Received: 4255744
Average Throughput: 32.4688Mbps
Average Delay: 27.7317ms
```

Output of Thompson Sampling example is similar.

