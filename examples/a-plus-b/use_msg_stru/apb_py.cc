#include <iostream>
#include <pybind11/pybind11.h>

#include <ns3/ai-module.h>
#include "apb.h"

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_apb_py_stru, m) {

    py::class_<EnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("a", &EnvStruct::env_a)
        .def_readwrite("b", &EnvStruct::env_b);

    py::class_<ActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("c", &ActStruct::act_c);

    py::class_<ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>>(m, "Ns3AiMsgInterface")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_recv_begin)
        .def("py_recv_end", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_recv_end)
        .def("py_send_begin", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_send_begin)
        .def("py_send_end", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_send_end)
        .def("py_get_finished", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::py_get_finished)
        .def_readwrite("m_single_cpp2py_msg", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::m_single_cpp2py_msg)
        .def_readwrite("m_single_py2cpp_msg", &ns3::Ns3AiMsgInterface<EnvStruct, ActStruct>::m_single_py2cpp_msg)
        ;

}


