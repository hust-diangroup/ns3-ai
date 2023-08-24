# ns3-ai Installation

This installation works on Ubuntu 22.04 and macOS 13.0 or higher.

## Requirements

1. Boost C++ libraries
    - Ubuntu: `sudo apt install libboost-all-dev`
    - macOS: `brew install boost`
2. Protocol buffers
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
3. pybind11
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

1. Clone this repository at `contrib/ai`

```shell
cd YOUR_NS3_DIRECTORY
git clone https://github.com/hust-diangroup/ns3-ai.git contrib/ai
```

2. Configure and build the `ai` library

```shell
./ns3 configure --enable-examples
./ns3 build ai
```

3. Setup Python interfaces. It's recommended to use a separate Conda environment 
for ns3-ai.

```shell
pip install -e contrib/ai/python_utils
pip install -e contrib/ai/model/gym-interface/py
```

4. Build the examples (optional)

All targets named `ns3ai_*` can be built separately.

```shell
# build all examples in all versions
./ns3 build ns3ai_apb_gym ns3ai_apb_msg_stru ns3ai_apb_msg_vec ns3ai_multibss ns3ai_rltcp_gym ns3ai_rltcp_msg ns3ai_ratecontrol_constant ns3ai_ratecontrol_ts ns3ai_ltecqi

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
./ns3 build ns3ai_ltecqi
```
