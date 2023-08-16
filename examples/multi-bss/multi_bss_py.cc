#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include <ns3/ns3-ai-module.h>
#include "multi-bss.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterface<Env, Act>::Cpp2PyMsgVector);
PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterface<Env, Act>::Py2CppMsgVector);

PYBIND11_MODULE(ns3ai_multibss_py, m) {
    
    py::class_<std::array<double, 5>>(m, "RxPowerArray")
        .def(py::init<>())
        .def("size", &std::array<double, 5>::size)
        .def("__len__", [](const std::array<double, 5> &arr) {
            return arr.size();
        })
        .def("__getitem__", [](const std::array<double, 5> &arr, uint32_t i){
            if (i >= arr.size()) {
                std::cerr << "Invalid index " << i << " for std::array, whose size is " << arr.size() << std::endl;
                exit(1);
            }
            return arr.at(i);
        })
        ;

    py::class_<Env>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("txNode", &Env::txNode)
        .def_readwrite("rxPower", &Env::rxPower)
        .def_readwrite("mcs", &Env::mcs)
        .def_readwrite("holDelay", &Env::holDelay)
        .def_readwrite("throughput", &Env::throughput)
        ;

    py::class_<Act>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("newCcaSensitivity", &Act::newCcaSensitivity)
        ;

    py::class_<ns3::Ns3AiMsgInterface<Env, Act>::Cpp2PyMsgVector>(m, "PyEnvVector")
        .def("resize", static_cast
             <void (ns3::Ns3AiMsgInterface<Env, Act>::Cpp2PyMsgVector::*)
                  (ns3::Ns3AiMsgInterface<Env, Act>::Cpp2PyMsgVector::size_type)>
             (&ns3::Ns3AiMsgInterface<Env, Act>::Cpp2PyMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterface<Env, Act>::Cpp2PyMsgVector::size)
        .def("__getitem__", [](ns3::Ns3AiMsgInterface<Env, Act>::Cpp2PyMsgVector&vec, uint32_t i) -> Env & {
                if (i >= vec.size()) {
                    std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            }, py::return_value_policy::reference)
        ;

    py::class_<ns3::Ns3AiMsgInterface<Env, Act>::Py2CppMsgVector>(m, "PyActVector")
        .def("resize", static_cast
             <void (ns3::Ns3AiMsgInterface<Env, Act>::Py2CppMsgVector::*)
                  (ns3::Ns3AiMsgInterface<Env, Act>::Py2CppMsgVector::size_type)>
             (&ns3::Ns3AiMsgInterface<Env, Act>::Py2CppMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterface<Env, Act>::Py2CppMsgVector::size)
        .def("__getitem__", [](ns3::Ns3AiMsgInterface<Env, Act>::Py2CppMsgVector&vec, uint32_t i) -> Act & {
                if (i >= vec.size()) {
                    std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            }, py::return_value_policy::reference)
        ;

    py::class_<ns3::Ns3AiMsgInterface<Env, Act>>(m, "Ns3AiMsgInterface")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin", &ns3::Ns3AiMsgInterface<Env, Act>::py_recv_begin)
        .def("py_recv_end", &ns3::Ns3AiMsgInterface<Env, Act>::py_recv_end)
        .def("py_send_begin", &ns3::Ns3AiMsgInterface<Env, Act>::py_send_begin)
        .def("py_send_end", &ns3::Ns3AiMsgInterface<Env, Act>::py_send_end)
        .def("py_get_finished", &ns3::Ns3AiMsgInterface<Env, Act>::py_get_finished)
        .def_readwrite("m_cpp2py_msg", &ns3::Ns3AiMsgInterface<Env, Act>::m_cpp2py_msg)
        .def_readwrite("m_py2cpp_msg", &ns3::Ns3AiMsgInterface<Env, Act>::m_py2cpp_msg)
        ;

}


