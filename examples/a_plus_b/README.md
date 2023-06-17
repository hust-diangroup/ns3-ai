# A Plus B example

## Introduction

In `ns3ai_apb` example, C++ side sets a vector in shared memory that containes 3 structures. Each structure contains 2 random numbers which can be seen as environment in RL. Python side gets the vector and compute the sum for each structure, and sets another vector in shared memory that contains the sums, which can be seen as action in RL. C++ then gets the actions and prints them. The procedure is repeated many times, as defined by the macro ENV_NUM on C++ side.

## Running the example

1. Clone the repository under `contrib`, checkout `improvements` branch

```bash
cd contrib
git clone https://github.com/ShenMuyuan/ns3-ai.git
cd ns3-ai
git checkout -b improvements origin/improvements
cd ../../
```

2. Use `./ns3` script to build the example. This builds both C++ and Python modules. An `.so` shared library will be placed in the example directory, which can be imported by Python.

```bash
./ns3 clean
./ns3 configure --enable-examples
./ns3 build ns3ai_apb
```

3. Run the example. You must run Python side first because Python script is the shared memory creator.

```bash
cd contrib/ns3-ai/examples/a_plus_b
python apb.py
```

4. Open another terminal and run C++ program.

```bash
./ns3 run ns3ai_apb
```

5. You should see output like this on C++ side. The numbers are random, we only care about the sum.

```
CPP env to set: 2,5;5,6;2,5;
Get act: 7;11;7;
CPP env to set: 10,10;7,7;6,8;
Get act: 20;14;14;
CPP env to set: 10,10;4,8;2,7;
Get act: 20;12;9;
......
```
