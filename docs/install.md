# ns3-ai Installation

This installation works on Ubuntu 22.04 and macOS 15.0 or higher.

## Requirements

1. Boost C++ libraries
    - Ubuntu: `sudo apt install libboost-all-dev`
    - macOS: `brew install boost`
2. Protocol buffers
    - Ubuntu: `sudo apt install libprotobuf-dev protobuf-compiler`
    - macOS: `brew install protobuf`
3. pybind11
    - Ubuntu: `sudo apt install pybind11-dev`
    - macOS: `brew install pybind11`
4. A Python virtual environment dedicated for ns3-ai (highly recommended)
    - For example, to use conda to create an environment named `ns3ai_env` with python version 3.11: `conda create -n ns3ai_env python=3.11`.

## General Setup

1. If a Python virtual environment is used, activate it. The absolute path to the Python executable of this environment, denoted as `<path-to-python>`, will be later passed to cmake via `ns3` script. You can find the path by first activate it and then `which python`.

2. Clone this repository at `contrib/ai`

```shell
cd YOUR_NS3_DIRECTORY
git clone https://github.com/hust-diangroup/ns3-ai.git contrib/ai
```

2. Configure and build the `ai` library

```shell
./ns3 configure --enable-examples -- -DPython_EXECUTABLE=<path-to-python> -DPython3_EXECUTABLE=<path-to-python>
./ns3 build ai
```

3. Setup Python interfaces.

```shell
pip install -e contrib/ai/python_utils
pip install -e contrib/ai/model/gym-interface/py
```

4. Build the examples (optional)

All targets named `ns3ai_*` can be built separately.

```shell
# build all examples in all versions
./ns3 build ns3ai_apb_gym ns3ai_apb_msg_stru ns3ai_apb_msg_vec ns3ai_multibss ns3ai_rltcp_gym ns3ai_rltcp_msg ns3ai_ratecontrol_constant ns3ai_ratecontrol_ts ns3ai_ltecqi_msg

# build A-Plus-B example
# with Gym interface
./ns3 build ns3ai_apb_gym
# with Message interface - struct based
./ns3 build ns3ai_apb_msg_stru
# with Message interface - vector based
./ns3 build ns3ai_apb_msg_vec

# build Multi-BSS example
# with Message interface - vector based
./ns3 build ns3ai_multibss

# build RL-TCP example
# with Gym interface
./ns3 build ns3ai_rltcp_gym
# with Message interface - struct based
./ns3 build ns3ai_rltcp_msg

# build Rate-Control examples
# constant rate example, with Message interface - struct based
./ns3 build ns3ai_ratecontrol_constant
# Thompson Sampling example, with Message interface - struct based
./ns3 build ns3ai_ratecontrol_ts

# build LTE-CQI example
# with Message interface - struct based
./ns3 build ns3ai_ltecqi_msg
```
