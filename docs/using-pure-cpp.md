# Using C++-based ML Frameworks

## TensorFlow C API (`libtensorflow`)

### Installation in ns3-ai

If `model/libtensorflow` directory exists and contains `lib` and `include`, targets using
TensorFlow C API is automatically enabled.

#### For Linux and Intel-based macOS

1. Download prebuilt library from [TensorFlow official website](https://www.tensorflow.org/install/lang_c).
2. Extract tarball under `model`.

```shell
cd YOUR_NS3_DIRECTORY
mkdir contrib/ai/model/libtensorflow
tar -xf PATH_TO_TARBALL -C contrib/ai/model/libtensorflow
```

#### For M1/M2-based macOS

1. The website does not provide arm64 prebuilt library. You need to 
build it, or get prebuilt library with `brew`. You can install 
`libtensorflow` with `brew`:

```shell
brew install tensorflow
```

2. Then copy the files to ns3-ai.

```shell
cd YOUR_NS3_DIRECTORY
cp -r /opt/homebrew/Cellar/libtensorflow/YOUR_VERSION/ contrib/ai/model/libtensorflow
```

### Usage

- Cmake target: `ns3ai_ltecqi_purecpp`

The Python API of TensorFlow provides the full functionality, while the C API 
is [in progress and incomplete](https://github.com/tensorflow/docs/blob/master/site/en/r1/guide/extend/bindings.md#current-status). 
As a result, the [LTE-CQI](../examples/lte-cqi) example, which uses LSTM and requires Gradients and
Neural Network library, cannot be rewritten into pure C++ version using `libtensorflow`. 

However, a simple example is provided in `examples/lte-cqi/pure_cpp` for checking whether 
`libtensorflow` is correctly installed. If it is, `./ns3 run ns3ai_ltecqi_purecpp` 
should successfully print TensorFlow's version. The full example using LSTM will be 
available when TensorFlow offers sufficient C API to developers.

## PyTorch C++ API (`libtorch`)

### Installation in ns3-ai

If `model/libtorch` directory exists and contains `lib` and `include`, targets using
PyTorch C++ API is automatically enabled.

#### For Linux and Intel-based macOS

1. Download prebuilt library from [PyTorch official website](https://pytorch.org/cppdocs/installing.html).
2. Unzip under `model`.

```shell
cd YOUR_NS3_DIRECTORY
unzip PATH_TO_ZIP -d contrib/ai/model/
```

#### For M1/M2-based macOS

1. The website does not provide arm64 prebuilt library. You need to
   build it, or get prebuilt library with `brew`. You can install
   `libtorch` with `brew`:

```shell
brew install pytorch
```

2. Then copy the files to ns3-ai.

```shell
cd YOUR_NS3_DIRECTORY
cp -r /opt/homebrew/Cellar/pytorch/YOUR_VERSION/ contrib/ai/model/libtorch
```

### Usage

- Cmake target: `ns3ai_rltcp_purecpp`

The [message-interface version of the original RL-TCP example](../examples/rl-tcp/use_msg) 
has been modified to run in pure C++, utilizing PyTorch C++ APIs. All RL 
functionalities are same with original, except for random seed settings that can 
lead to different results. Running `./ns3 run ns3ai_rltcp_purecpp` should by 
default apply deep Q-learning algorithm to choose TCP parameters, with states 
and actions printing in the console.
