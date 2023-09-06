# Message Interface

## Introduction

Message interface is lower-level compared to Gym interface. In fact, Gym interface is
based on message interface for interprocess communication. The message interface has
send and receive functions just like a typical message queue.

When there is existing `struct` or `std::vector` to be shared with Python, it's easier
to employ message interface than Gym interface because you don't need to convert the
data to Gym spaces. Also, both C++ and Python side access the data directly, which
saves the serialization/deserialization time especially for frequent interaction.

There are two versions of the message interface: **struct-based** and **vector-based**.
- The struct-based version allows for the transfer of structures between C++ and Python,
making it ideal for handling small amounts of data.
- The vector-based version enables the transfer of vectors between C++ and Python,
making it more suitable for dealing with large amounts of data.

## Tutorial

The following tutorial of this interface is based on the [A-Plus-B](../../examples/a-plus-b) example.

In A-Plus-B example, with **struct-based** message interface, C++ sends two numbers to
Python, and Python responds to C++ with one sum. With **vector-based** message
interface, C++ sends a vector of two numbers to Python, and Python responds to C++
with a vector of sums.

### Struct-based message interface

#### C++ side

We have defined two structs, `EnvStruct` for the two numbers `env_a` and `env_b`,
and `ActStruct` for their sum `act_c` that needs to be calculated:

```c++
struct EnvStruct
{
    uint32_t env_a;
    uint32_t env_b;
}

struct ActStruct
{
    uint32_t act_c;
}
```

Start by acquiring a message interface:

```c++
Ns3AiMsgInterface::Get()->SetIsMemoryCreator(false);
Ns3AiMsgInterface::Get()->SetUseVector(false);
Ns3AiMsgInterface::Get()->SetHandleFinish(true);
Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct> *msgInterface =
    Ns3AiMsgInterface::Get()->GetInterface<EnvStruct, ActStruct>();
```

The above lines sets some attributes for the message interface:
- `SetIsMemoryCreator`: Controls whether or not this side initializes the interface.
Because the shared memory is a named region, after the creator initializes the
segment with a unique name, the other side can access the segment with that name.
- `SetUseVector`: Controls whether to use `std::vector` or not.
- `SetHandleFinish`: Controls whether or not a simple protocol is enabled, which
notifies Python side when C++ side interface is destroyed (possibly due to ns-3
program exit). This is useful for all applications using the message interface,
because Python side is always unaware of simulation ending. For Gym interface on
top of message interface, this function should not be enabled because Gym interface
has its own protocol dealing with simulation ending.

Because shared memory segment is created by Python side, the C++ side
interface won't create it. We use struct rather than vector, and the message interface
requires the protocol dealing with simulation ending. Therefore, the settings
should be `false`, `false` and `true`.

After settings, the message interface instance is obtained with `GetInterface`
template function, with `EnvStruct` and `ActStruct` as template arguments. It
is singleton-based, so changing the settings and getting another interface in one
process is not possible.

Then, interact with Python (some initialization code is skipped). The interface
is simple and intuitive. To set `temp_a` and `temp_b` into shared memory, just write
them into the structure obtained by `GetCpp2PyStruct`. To get the sum, just read
from the structure obtained by `GetPy2CppStruct`.

```c++
msgInterface->CppSendBegin();
std::cout << "set: ";
uint32_t temp_a = distrib(gen);
uint32_t temp_b = distrib(gen);
std::cout << temp_a << "," << temp_b << ";";
msgInterface->GetCpp2PyStruct()->env_a = temp_a;
msgInterface->GetCpp2PyStruct()->env_b = temp_b;
std::cout << "\n";
msgInterface->CppSendEnd();

msgInterface->CppRecvBegin();
std::cout << "get: ";
std::cout << msgInterface->GetPy2CppStruct()->act_c;
std::cout << "\n";
msgInterface->CppRecvEnd();
```

Note that before and after read or write, several **synchronization** functions
are needed. `CppSendBegin` and `CppSendEnd` are for sending to Python, and `CppRecvBegin`
and `CppRecvEnd` are for receiving from Python. There are similar functions for
Python side. These functions are based on semaphore, which requires strict sequence.
If C++ side is receiving at the beginning, then Python side must be sending at the
beginning. The [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-sema.pdf) book
has a good introduction of semaphores.

#### Python side

Python side interface is a **binding** of the C++ side interface. Python binding means
the Python side reuses the C++ code to have the same capabilities as C++. Because the C++
code is template-based, Python binding varies with the data structure that is being
shared, and every example is unique.

The bindings in ns3-ai is created with pybind11. For detailed instructions on writing
Python bindings, check out [pybind11 documentation](https://pybind11.readthedocs.io/en/stable/index.html).
However, most bindings have similar style and fixed components. A Plus B's binding code
is a good boilerplate, you can just modify a few lines and apply to another program.

Basically, you can follow these steps:

1. Declare the Python module to create. In this example, `ns3ai_apb_py_stru` will be the
import name of the binding module.

```c++
PYBIND11_MODULE(ns3ai_apb_py_stru, m) {
    ...
}
```

2. Bind the structures. Every item that Python may access (not necessarily all items)
need to be mentioned in the code. The names in Python can be different from that in
C++. For instance, `a` in Python is `env_a` in C++.

```c++
py::class_<EnvStruct>(m, "PyEnvStruct")
    .def(py::init<>())
    .def_readwrite("a", &EnvStruct::env_a)
    .def_readwrite("b", &EnvStruct::env_b);

py::class_<ActStruct>(m, "PyActStruct")
    .def(py::init<>())
    .def_readwrite("c", &ActStruct::act_c);
```

3. Bind the C++ class. Every function or member that Python may use (not necessarily
all) need to be mentioned in the binding code. In this binding, C++-only methods such
as `CppSendBegin` is excluded because Python side never use it.

```c++
py::class_<ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>>(m, "Ns3AiMsgInterfaceImpl")
    .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
    .def("PyRecvBegin",
         &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::PyRecvBegin)
    .def("PyRecvEnd",
         &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::PyRecvEnd)
    .def("PySendBegin",
         &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::PySendBegin)
    .def("PySendEnd",
         &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::PySendEnd)
    .def("PyGetFinished",
         &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::PyGetFinished)
    .def("GetCpp2PyStruct",
         &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::GetCpp2PyStruct,
         py::return_value_policy::reference)
    .def("GetPy2CppStruct",
         &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::GetPy2CppStruct,
         py::return_value_policy::reference)
    ;
```

To reuse this binding code on another example using struct-based message interface,
you only need to change the module name, structure content and the template parameters.

**Note: The binding module is configured by CMake to generate Python-compatible shared
library at the source directory, rather than adding a Python module. Therefore, the
Python script importing the binding should be also located in the source directory.**

In the A-Plus-B Python script, import the binding module and `Experiment` object from
`ns3ai_utils` module, and acquire the message interface:

```python
import ns3ai_apb_py_stru as py_binding
from ns3ai_utils import Experiment

exp = Experiment("ns3ai_apb_msg_stru", "../../../../../", py_binding,
                 handleFinish=True)
msgInterface = exp.run(show_output=True)
```

The initialization of `Experiment` creates a region of shared memory according to the
options given. The first three parameters are:
- `targetName`: Cmake target name of the executable.
- `ns3Path`: Relative path to get to the `ns3` script.
- `msgModule`: Alias of the Python binding module.

In the keyword option part, `handleFinish=True` is given, like C++ side. By default,
using vector is turned off.

The `exp.run` starts the `ns3` script subprocess and its C++ subprocess which do the
simulation. The message interface is returned for data transfer and synchronization,
and the APIs are very similar to C++ side:

```python
# receive from C++ side
msgInterface.PyRecvBegin()
if msgInterface.PyGetFinished():
    break
# calculate the sum
temp = msgInterface.GetCpp2PyStruct().a + msgInterface.GetCpp2PyStruct().b
msgInterface.PyRecvEnd()

# send to C++ side
msgInterface.PySendBegin()
msgInterface.GetPy2CppStruct().c = temp
msgInterface.PySendEnd()
```

Most Python scripts in ns3-ai examples adopts the `try ... except ... else ... finally ...`
syntax, in order for better exception handling and proper cleaning before exit.
To clean `Experiment` instances, just explicitly call `del`:

```python
finally:
    print("Finally exiting...")
    del exp
```

### Vector-based message interface

Unlike the other two versions, this vector-based version of A-Plus-B example
demonstrates the sharing of numbers and their sums using `std::vector`.

#### C++ side

Start with this setting, which turns on the vector usage.

```c++
Ns3AiMsgInterface::Get()->SetIsMemoryCreator(false);
Ns3AiMsgInterface::Get()->SetUseVector(true);
Ns3AiMsgInterface::Get()->SetHandleFinish(true);
Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct> *msgInterface =
    Ns3AiMsgInterface::Get()->GetInterface<EnvStruct, ActStruct>();
```

Then, interact with Python (some initialization code is skipped). The interface
is simple and intuitive. To set `temp_a`s and `temp_b`s into shared memory, just write
them into the vector obtained by `GetCpp2PyVector`. To get the sums, just read
from the vector obtained by `GetPy2CppVector`.

```c++
msgInterface->CppSendBegin();
std::cout << "set: ";
for (int j = 0; j < APB_SIZE; ++j) {
    uint32_t temp_a = distrib(gen);
    uint32_t temp_b = distrib(gen);
    std::cout << temp_a << "," << temp_b << ";";
    msgInterface->GetCpp2PyVector()->at(j).env_a = temp_a;
    msgInterface->GetCpp2PyVector()->at(j).env_b = temp_b;
}
std::cout << "\n";
msgInterface->CppSendEnd();

msgInterface->CppRecvBegin();
std::cout << "get: ";
for (ActStruct j: *msgInterface->GetPy2CppVector()) {
    std::cout << j.act_c << ";";
}
std::cout << "\n";
msgInterface->CppRecvEnd();
```

The synchronization method of the vector version is the same as that of struct-based
version.

#### Python side

Binding vectors is different from binding structs, but the binding code is
again a boilerplate and can be easily reused in other projects.

There are two major additions compared to the previous struct-based binding:
1. Opaque vectors: use the macro `PYBIND11_MAKE_OPAQUE` to treat vectors as
references instead of copying them to Python side, making Python module faster.

```c++
PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::Cpp2PyMsgVector);
```

2. Vector binding: essential methods such as `resize`, `__len__` and
`__getitem__` needs to be implemented.

```c++
py::class_<ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::Cpp2PyMsgVector>(m, "PyEnvVector")
    .def("resize", static_cast
         <void (ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::Cpp2PyMsgVector::*)
              (ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::Cpp2PyMsgVector::size_type)>
         (&ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::Cpp2PyMsgVector::resize))
    .def("__len__", &ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::Cpp2PyMsgVector::size)
    .def("__getitem__", [](ns3::Ns3AiMsgInterfaceImpl<EnvStruct, ActStruct>::Cpp2PyMsgVector&vec, uint32_t i) -> EnvStruct & {
        if (i >= vec.size()) {
            std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
            exit(1);
        }
        return vec.at(i);
    }, py::return_value_policy::reference)
    ;
```

In the Python script, import the binding module and `Experiment` object from
`ns3ai_utils` module, and acquire the message interface:

```python
exp = Experiment("ns3ai_apb_msg_vec", "../../../../../", py_binding,
                 handleFinish=True, useVector=True, vectorSize=APB_SIZE)
msgInterface = exp.run(show_output=True)
```

This time the `useVector=True` option must be specified, because `useVector` defaults
to `False`. The `vectorSize=APB_SIZE` option sets the length that the vector will be
resized to.

Interact with C++ side:

```python
# receive from C++ side
msgInterface.PyRecvBegin()
if msgInterface.PyGetFinished():
    break

# send to C++ side
msgInterface.PySendBegin()
for i in range(len(msgInterface.GetCpp2PyVector())):
    # calculate the sums
    msgInterface.GetPy2CppVector()[i].c = msgInterface.GetCpp2PyVector()[i].a + msgInterface.GetCpp2PyVector()[i].b
msgInterface.PyRecvEnd()
msgInterface.PySendEnd()
```

`PySendBegin` is called before `PyRecvEnd` for the convenience of saving a temp
variable. This won't cause errors because C++ is not posting on the semaphore
`m_py2cppEmptyCount` which `PySendBegin` is waiting until `PySendEnd` completes.

Remember to destroy the interface:

```python
finally:
    print("Finally exiting...")
    del exp
```
