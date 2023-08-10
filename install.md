# ns3-ai Installation

This installation works on Ubuntu 22.04 and macOS 13.0 or higher.

## Prerequisites

- Boost C++ libraries
    - Ubuntu: `sudo apt install libboost-all-dev`
    - macOS: `brew install boost`
- Protocol buffers
    - Ubuntu:
    ```shell
    # Recommended
    sudo apt install libprotobuf-dev protobuf-compiler
    
    # Try building from source if the above installation causes cmake error
    git clone https://github.com/protocolbuffers/protobuf.git
    cd protobuf
    git submodule update --init --recursive
    cmake -S . -B build -Dprotobuf_BUILD_SHARED_LIBS=ON
    cmake --build build
    sudo cmake --install build
    ```
    - macOS: `brew install protobuf`
- pybind11
    - Ubuntu:
    ```shell
    # Recommended
    sudo apt install pybind11-dev
    
    # Try building from source if the above installation causes cmake error
    git clone https://github.com/pybind/pybind11.git
    cd pybind11
    cmake -S . -B build
    cmake --build build
    sudo cmake --install build
    ```
    - macOS: `brew install pybind11`

## General Setup

1. Clone this repository and configure

```shell
cd contrib
git clone https://github.com/ShenMuyuan/ns3-ai.git
cd ns3-ai
git checkout -b improvements origin/improvements
cd ../../
./ns3 configure --enable-examples
```

2. Build the examples

When building examples, the `ns3-ai` library is built automatically.

```shell
# build A-Plus-B
./ns3 build ns3ai_apb_gym ns3ai_apb_msg_stru ns3ai_apb_msg_vec
# build RL-TCP
./ns3 build ns3ai_rltcp_msg ns3ai_rltcp_gym
# build Rate-Control
./ns3 build ns3ai_ratecontrol_constant ns3ai_ratecontrol_ts
# build LTE-CQI
./ns3 build ns3ai_ltecqi
# or build them altogether
./ns3 build ns3ai_apb_gym ns3ai_apb_msg_stru ns3ai_apb_msg_vec ns3ai_rltcp_msg ns3ai_rltcp_gym ns3ai_ratecontrol_constant ns3ai_ratecontrol_ts ns3ai_ltecqi
```

3. Setup Gym interface

```shell
cd cd contrib/ns3-ai/model/gym-interface/py
pip install -e .
```
