#include <pybind11/pybind11.h>

#include <ns3/ns3-ai-rl.h>
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

    py::class_<ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>>(m, "Ns3AiRl")
        .def(py::init<uint32_t, bool, bool, const char*, const char*, const char*, const char*>())
        .def("get_env_begin", &ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::get_env_begin)
        .def("get_env_end", &ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::get_env_end)
        .def("set_act_begin", &ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::set_act_begin)
        .def("set_act_end", &ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::set_act_end)
        .def("is_finished", &ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::is_finished)
        .def_readwrite("m_single_env", &ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::m_single_env)
        .def_readwrite("m_single_act", &ns3::Ns3AiRl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::m_single_act)
        ;

}

