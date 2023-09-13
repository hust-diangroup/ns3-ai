# Rate Control Example

## Introduction

There are existing models of constant rate and Thompson sampling algorithms in Wi-Fi module. Here we reimplement
them in Python to show how to develop a new rate control algorithm for the Wi-Fi module using ns3-ai.

### Cmake targets

- `ns3ai_ratecontrol_constant`: The constant rate example using struct-based message interface.
- `ns3ai_ratecontrol_ts`: The Thompson Sampling example using struct-based message interface.

## Algorithms

We provide two rate control algorithms that dynamically adjusts MCS (Modulation and Coding
Scheme) and NSS (Number of Spacial Streams):

- Constant rate: the MCS and NSS do not change, i.e., Python side write the received states
without modification into the corresponding actions.
- Thompson Sampling: changes the MCS and NSS according to Thompson Sampling algorithm.
  - In TS algorithm each action has parameters alpha and beta attached to it (independent
with other actions), used to estimate probability of success.
The goal is to find the action with the highest estimated probability of success.
  - TS algorithm estimates by sampling from posterior beta distribution (defined with alpha and beta).
By choosing the highest estimated probability, TS probes potentially good actions, avoids
unuseful actions, and finally converge at the optimal one.

### Input

- `transmitStreams`: Number of spacial streams in use.
- `supportedStreams`: Number of spacial streams supported.
- `mcs`: Current MCS index.

### Output

- `nss`: Number of spacial streams to use.
- `next_mcs`: MCS index to use.

## Running the example

### Constant rate

1. [Setup ns3-ai](../../docs/install.md)
2. Build C++ executable & Python bindings

```shell
cd YOUR_NS3_DIRECTORY
./ns3 build ns3ai_ratecontrol_constant
```

3. Run Python script

```shell
pip install -r contrib/ai/examples/rate-control/requirements.txt
cd contrib/ai/examples/rate-control/constant
python ai_constant_rate.py
```

### Thompson Sampling

1. [Setup ns3-ai](../../docs/install.md)
2. Build C++ executable & Python bindings

```shell
cd YOUR_NS3_DIRECTORY
./ns3 build ns3ai_ratecontrol_ts
```

3. Run Python script

```shell
pip install -r contrib/ai/examples/rate-control/requirements.txt
cd contrib/ai/examples/rate-control/thompson-sampling
python ai_thompson_sampling.py
```

## Results

For Constant Rate example, you will see:

```
nWifi: 3, RAA Algorithm: AiConstantRate, duration: 5
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
Delay: 23.1909ms, Throughput: 1.9693Mbps
Delay: 40.2246ms, Throughput: 3.56537Mbps
Delay: 72.5368ms, Throughput: 3.6908Mbps
Delay: 103.885ms, Throughput: 3.83331Mbps
Delay: 128.402ms, Throughput: 3.47961Mbps
Delay: 168.145ms, Throughput: 3.67218Mbps
Delay: 153.106ms, Throughput: 3.53912Mbps
Delay: 151.033ms, Throughput: 3.71582Mbps
Delay: 133.28ms, Throughput: 3.85162Mbps
Delay: 135.942ms, Throughput: 3.83331Mbps
Delay: 96.4398ms, Throughput: 3.50616Mbps
Delay: 122.525ms, Throughput: 3.74359Mbps
Delay: 145.24ms, Throughput: 3.55621Mbps
Delay: 154.465ms, Throughput: 3.94135Mbps
Delay: 139.596ms, Throughput: 3.49548Mbps
Delay: 111.297ms, Throughput: 3.59436Mbps
Delay: 100.204ms, Throughput: 3.63007Mbps
Delay: 109.981ms, Throughput: 3.75153Mbps
Delay: 118.734ms, Throughput: 3.91785Mbps
Delay: 111.004ms, Throughput: 3.82538Mbps
Delay: 120.409ms, Throughput: 3.78845Mbps
Delay: 116.787ms, Throughput: 3.6908Mbps
Delay: 123.956ms, Throughput: 3.6618Mbps
Delay: 122.287ms, Throughput: 3.87421Mbps
Delay: 132.692ms, Throughput: 3.71063Mbps
Delay: 124.487ms, Throughput: 3.8504Mbps
Delay: 128.176ms, Throughput: 3.55621Mbps
Delay: 114.337ms, Throughput: 3.79761Mbps
Delay: 134.766ms, Throughput: 3.56812Mbps
Delay: 143.206ms, Throughput: 3.77533Mbps
Delay: 121.313ms, Throughput: 3.66302Mbps
Delay: 141.094ms, Throughput: 3.7265Mbps
Delay: 140.505ms, Throughput: 3.58124Mbps
Delay: 146.769ms, Throughput: 3.89404Mbps
Delay: 141.993ms, Throughput: 3.89008Mbps
Delay: 126.657ms, Throughput: 3.69995Mbps
Delay: 145.003ms, Throughput: 3.80432Mbps
Delay: 150.105ms, Throughput: 3.61298Mbps
Delay: 138.559ms, Throughput: 3.62213Mbps
Delay: 138.407ms, Throughput: 3.80951Mbps
Delay: 137.887ms, Throughput: 3.87817Mbps
Delay: 171.286ms, Throughput: 3.66058Mbps
Delay: 144.06ms, Throughput: 3.57727Mbps
Delay: 183.974ms, Throughput: 3.46527Mbps
Delay: 165.172ms, Throughput: 3.51532Mbps
Delay: 180.612ms, Throughput: 3.65784Mbps
Delay: 179.146ms, Throughput: 3.66302Mbps
Delay: 163.219ms, Throughput: 3.71582Mbps
Delay: 185.042ms, Throughput: 3.66974Mbps
Delay: 162.097ms, Throughput: 3.9151Mbps
Delay: 170.27ms, Throughput: 3.86627Mbps
Delay: 139.565ms, Throughput: 3.92029Mbps
......
```

TS's output has the same format, but the delay and throughput is much better.

