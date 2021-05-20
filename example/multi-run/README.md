This is a very simple but useful example for the ns3-ai model to illustrate the data exchange between python-based AI frameworks and ns-3. In this example, we have two variable a and b in ns-3, and then put them into the shared memory using python to calculate c = a + b. Finally, we put back c to the ns-3. 

### Usage
Copy this example to scratch:
```shell
cp -r contrib/ns3-ai/example/multi-run scratch/
```

Run the code (Note the python script can start the ns-3 script automatically, so you do not need to start it by yourself):

```shell
cd scratch/multi-run/

python3 run.py
```
### Data Structure

#### Environment
We establish the environment (`Env`) for ns-3 and python and point to the same shared memory using the ns3-ai model. The variables a and b are shared through the `Env`.  
Python
```Python
# Shared memory to store a and b
class Env(Structure):
    _pack_ = 1
    _fields_ = [
        ('a', c_int),
        ('b', c_int)
    ]
```

ns-3
```c++
# Shared memory to store a and b
struct Env
{
    int a;
    int b;
}Packed;
```

#### Action
The action (`Act`) is the result that is calculated by python and put back to ns-3 with the shared memory.

Python

```Python
# Shared memory to store action c
class Act(Structure):
    _pack_ = 1
    _fields_ = [
        ('c', c_int)
    ]
```

ns-3

```c++
# Shared memory to store action c
struct Act
{
    int c;
}Packed;
```
### Class APB 
Now we consider the class APB (a plus b) in the ns-3 simulation code.

```c++
class APB : public Ns3AIRL<Env, Act>
{
public:
    APB(uint16_t id);
    int Func(int a, int b);
};
/* 
input: 
    uint16_t id: shared memory id, should be the same in python and ns-3
function:
    link the shared memory with the id and set the operation lock
*/
APB::APB(uint16_t id) : Ns3AIRL<Env, Act>(id) 
{ 
    // Set the operation lock (even for ns-3 and odd for python)
    SetCond(2, 0); 
}
/*
inputs：
    two variable a and b
function:
    put a and b into the shared memory;
    wait for the python to calculate the result c = a + b;
    get the result c from shared memory;
output:
    result c = a + b calculated by python
*/
int APB::Func(int a, int b)
{
    // Acquire the Env memory for writing 
    auto env = EnvSetterCond();
    // Set the shared memory
    env->a = a;
    env->b = b;
    //Release the memory and update conters
    SetCompleted();
    
    // Acquire the Act memory for reading
    auto act = ActionGetterCond();
    // Get the result
    int ret = act->c;
    //Release the memory and update conters
    GetCompleted();

    return ret;
}
```

The main function is quite simple to understand that just init the Env and put the variables.

### Python script
A very convenient way we use here is to use python directly to establish the ns-3 script.  
Set up the ns-3 environment

```Python
# Experiment(self, shmKey, memSize, programName, path)
exp = Experiment(1234, 4096, 'multi-run', '../../')
```

You need to change the name and path according to the different ns-3 scripts' names.  
Establish the envrionments

```Python
# Reset the environment
exp.reset()
# Link the shared memory block
rl = Ns3AIRL(2333, Env, Act)
# run the ns-3 script
# Enable logs to std from ns-3: pro = exp.run(show_output=True)
# Add settings for ns-3: pro = exp.run(setting='--xx=xx')
pro = exp.run()
# While loop: the program will continue to monitor the shared memory for the update. At each time Env updates, it will return a new action.
while not rl.isFinish():
    with rl as data:
        if data == None:
            break
        # AI algorithms here and put the data back to the action
        data.act.c = data.env.a+data.env.b
# Wait the ns-3 to stop
pro.wait()
```