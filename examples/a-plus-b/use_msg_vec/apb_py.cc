#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include <ns3/ai-module.h>
#include "apb.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Cpp2PyMsgVector);
PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Py2CppMsgVector);

PYBIND11_MODULE(ns3ai_apb_py, m) {

    py::class_<EnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("a", &EnvStruct::env_a)
        .def_readwrite("b", &EnvStruct::env_b);

    py::class_<ActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("c", &ActStruct::act_c);

    py::class_<ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Cpp2PyMsgVector>(m, "PyEnvVector")
        .def("resize", static_cast
             <void (ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Cpp2PyMsgVector::*)
                  (ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Cpp2PyMsgVector::size_type)>
             (&ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Cpp2PyMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Cpp2PyMsgVector::size)
        .def("__getitem__", [](ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Cpp2PyMsgVector&vec, uint32_t i) -> EnvStruct & {
            if (i >= vec.size()) {
                std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                exit(1);
            }
            return vec.at(i);
        }, py::return_value_policy::reference)
        ;

    py::class_<ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Py2CppMsgVector>(m, "PyActVector")
        .def("resize", static_cast
             <void (ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Py2CppMsgVector::*)
                  (ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Py2CppMsgVector::size_type)>
             (&ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Py2CppMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Py2CppMsgVector::size)
        .def("__getitem__", [](ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::Py2CppMsgVector&vec, uint32_t i) -> ActStruct & {
            if (i >= vec.size()) {
                std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                exit(1);
            }
            return vec.at(i);
        }, py::return_value_policy::reference)
        ;

    py::class_<ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>>(m, "Ns3AiMsgInterface")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_recv_begin)
        .def("py_recv_end", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_recv_end)
        .def("py_send_begin", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_send_begin)
        .def("py_send_end", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_send_end)
        .def("py_get_finished", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_get_finished)
        .def_readwrite("m_cpp2py_msg", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::m_cpp2py_msg)
        .def_readwrite("m_py2cpp_msg", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::m_py2cpp_msg)
        ;

}


