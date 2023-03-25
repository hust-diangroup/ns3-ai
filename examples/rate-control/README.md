# Rate Control Example
There are existing models of constant rate and Thompson sampling algorithms in Wi-Fi module. Here we reimplement 
them in Python to show how to develop a new rate control algorithm for the Wi-Fi module using ns3-ai.

## Thompson Sampling Algorithm

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

## Interaction between ns-3 and ns3-ai

In general, C++ code stores the environment in shared memory (for C++ side, write only,
for Python side, read only). Python code updates action to another
part of shared memory (for Python side, write only, for C++ side, read only).

- C++ side (ns-3)

The type of environment indicates different events (e.g. 
initialization, tx start, tx success, tx failure) that occurs 
in ns-3 simulation.

For example, in `ai-thompson-sampling-wifi-manager.cc`, type number 
0x08 tells Python side that function DoGetDataTxVector is being called 
which requires updating MCS:

```c++
  // set input, type 0x08
  auto env = m_ns3ai_mod->EnvSetterCond ();
  env->type = 0x08;
  env->managerId = m_ns3ai_manager_id;
  env->stationId = station->m_ns3ai_station_id;
  m_ns3ai_mod->SetCompleted ();

  // get output
  auto act = m_ns3ai_mod->ActionGetterCond ();
  WifiMode mode = station->m_mcsStats.at (act->res).mode;
  uint8_t nss = act->stats.nss;
  uint16_t channelWidth = std::min (act->stats.channelWidth, GetPhy ()->GetChannelWidth ());
  uint16_t guardInterval = act->stats.guardInterval;
  m_ns3ai_mod->GetCompleted ();
```

- Python side (ns3-ai)

Python code handles environment in shared memory according to the type number.
For example, when a transmission is over (data failed, type 0x05, or data OK, type 0x06), 
statistics will be updated and an optimal MCS is selected for next transmission.

From `ai_thompson_sampling.py`:

```
    def do(self, env: AiThompsonSamplingEnv, act: AiThompsonSamplingAct) -> AiThompsonSamplingAct:
        ...
        elif env.type == 0x05:  # DoReportDataFailed
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            sta.DoReportDataFailed(env.decay.decay, env.decay.now)
            man.UpdateNextMode(sta, env.decay.decay, env.decay.now)
            act.stationId = env.stationId
        ...
        elif env.type == 0x06:  # DoReportDataOk
            man = self.wifiManager[env.managerId]
            sta = self.wifiStation[env.stationId]
            sta.DoReportDataOk(env.decay.decay, env.decay.now)
            man.UpdateNextMode(sta, env.decay.decay, env.decay.now)
            act.stationId = env.stationId
```

## Build and run

Copy this example to scratch:

```shell
cp -r contrib/ns3-ai/examples/rate-control scratch/
cd scratch/rate-control
```

### 1. Constant Rate Control

```shell
python3 ai_constant_rate.py
```

### 2. Thompson Sampling Rate Control

```shell
python3 ai_thompson_sampling.py
```

## Output

Default simulation time is 5 seconds (configured via Command-Line argument `duration`). 
The delay and throughput is printed every `statInterval` seconds, by default is 0.1s.