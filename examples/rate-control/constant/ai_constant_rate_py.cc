#include <pybind11/pybind11.h>

#include <ns3/ai-module.h>
#include "ai-constant-rate-wifi-manager.h"

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_ratecontrol_constant_py, m) {

    py::class_<ns3::AiConstantRateEnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("transmitStreams", &ns3::AiConstantRateEnvStruct::transmitStreams)
        .def_readwrite("supportedStreams", &ns3::AiConstantRateEnvStruct::supportedStreams)
        .def_readwrite("mcs", &ns3::AiConstantRateEnvStruct::mcs)
        ;

    py::class_<ns3::AiConstantRateActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("nss", &ns3::AiConstantRateActStruct::nss)
        .def_readwrite("next_mcs", &ns3::AiConstantRateActStruct::next_mcs)
        ;

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                     ns3::AiConstantRateActStruct>::py_recv_begin)
        .def("py_recv_end",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                     ns3::AiConstantRateActStruct>::py_recv_end)
        .def("py_send_begin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                     ns3::AiConstantRateActStruct>::py_send_begin)
        .def("py_send_end",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                     ns3::AiConstantRateActStruct>::py_send_end)
        .def("py_get_finished",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                     ns3::AiConstantRateActStruct>::py_get_finished)
        .def_readwrite("m_single_cpp2py_msg", &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::m_single_cpp2py_msg)
        .def_readwrite("m_single_py2cpp_msg", &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::m_single_py2cpp_msg)
        ;

}

