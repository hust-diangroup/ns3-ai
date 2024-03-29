# A-Plus-B example

## Introduction

In this example, C++ side starts by setting 2 random numbers between 0 and 10 in shared memory. Then, Python side gets the
numbers and sets the sum of the numbers in shared memory (in another region). Finally, C++ gets the sum that Python set.
The procedure is analogous to C++ passing RL states to Python and Python passing RL actions back to C++, and is
repeated many times.

We created this example to demonstrate the basic usage of ns3-ai's two interfaces: Gym interface and message interface.

### Cmake targets

- `ns3ai_apb_gym`: A-Plus-B using Gym interface
- `ns3ai_apb_msg_stru`: A-Plus-B using message interface (struct-based)
- `ns3ai_apb_msg_vec`: A-Plus-B using message interface (vector-based)

## Running the example

This example can be run using the following interfaces:

### Gym interface

1. [Setup ns3-ai](../../docs/install.md)
2. Build C++ executable & Python bindings

```shell
cd YOUR_NS3_DIRECTORY
./ns3 build ns3ai_apb_gym
```

3. Run Python script

```bash
cd contrib/ai/examples/a-plus-b/use-gym
python apb.py
```

### Message interface (struct-based)

1. [Setup ns3-ai](../../docs/install.md)
2. Build C++ executable & Python bindings

```shell
cd YOUR_NS3_DIRECTORY
./ns3 build ns3ai_apb_msg_stru
```

3. Run Python script

```bash
cd contrib/ai/examples/a-plus-b/use-msg-stru
python apb.py
```

### Message interface (vector-based)

1. [Setup ns3-ai](../../docs/install.md)
2. Build C++ executable & Python bindings

```shell
cd YOUR_NS3_DIRECTORY
./ns3 build ns3ai_apb_msg_vec
```

3. Run Python script

```bash
cd contrib/ai/examples/a-plus-b/use-msg-vec
python apb.py
```

## Results

For Gym interface and Message interface (struct-based), the terminal will
repeatedly print two random numbers generated by C++ and their sum
calculated by Python:

```text
set: 4,10;
get: 14;
set: 10,8;
get: 18;
set: 5,1;
get: 6;
set: 10,6;
get: 16;
......
```

For Message interface (vector-based), the terminal will repeatedly print
a vector of two random numbers, with default size = 3, generated by C++,
and the vector of the sums, also size=3, calculated by Python:

```text
set: 1,1;2,4;1,4;
get: 2;6;5;
set: 6,10;10,2;1,2;
get: 16;12;3;
set: 4,4;7,10;6,4;
get: 8;17;10;
set: 10,3;5,8;10,9;
get: 13;13;19;
......
```
