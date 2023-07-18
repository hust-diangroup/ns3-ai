# A Plus B example

## Introduction

In `ns3ai_apb` example, C++ side sets a vector in shared memory that containes 3 structures. Each structure contains 2
random numbers which can be seen as environment in RL. Python side gets the vector and compute the sum for each
structure, and sets another vector in shared memory that contains the sums, which can be seen as action in RL. C++ then
gets the actions and prints them. The procedure is repeated many times, as defined by the macro ENV_NUM on C++ side.

## Running the example (msg interface)

1. [General setup](https://github.com/ShenMuyuan/ns3-ai/tree/improvements#general-setup)

2. Run the example.

- You must run Python side first, because Python script is the shared memory creator.

```bash
cd contrib/ns3-ai/examples/a_plus_b/use_msg
python apb.py
```

- When you see the message `Created message interface, waiting for C++ side to send initial environment...`, Open
  another terminal and run C++ side.

```bash
./ns3 run ns3ai_apb_msg
```

## Running the example (Gym interface)

1. [General setup](https://github.com/ShenMuyuan/ns3-ai/tree/improvements#general-setup)

2. Run the example.

- You must run Python side first, because Python script is the shared memory creator.

```bash
cd contrib/ns3-ai/examples/a_plus_b/use_gym
python apb.py
```

- When you see the message `Created message interface, waiting for C++ side to send initial environment...`, Open
  another terminal and run C++ side.

```bash
./ns3 run ns3ai_apb_gym
```

## Output

No matter which interface you use, you will see two numbers set by C++ side and their sum get from Python side. (Msg
interface can take advantage of vectors. To demonstrate this, 3 pairs of numbers are set in C++ side and 3 sums are
obtained from Python side.)
