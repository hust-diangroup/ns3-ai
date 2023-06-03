#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include <ns3/ns3-ai-new-rl.h>
#include "apb.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<EnvStruct>);
PYBIND11_MAKE_OPAQUE(std::vector<ActStruct>);

PYBIND11_MODULE(ns3ai_apb_py, m) {

    py::class_<EnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("a", &EnvStruct::env_a)
        .def_readwrite("b", &EnvStruct::env_b);

    py::class_<ActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("c", &ActStruct::act_c);

    py::bind_vector<std::vector<EnvStruct>>(m, "PyEnvVector");
    py::bind_vector<std::vector<ActStruct>>(m, "PyActVector");

    py::class_<ns3::NS3AIRL<EnvStruct, ActStruct>>(m, "NS3AIRL")
        .def(py::init<uint32_t, bool, const char*, const char*, const char*, const char*>())
        .def("get_env", &ns3::NS3AIRL<EnvStruct, ActStruct>::get_env)
        .def("set_act", &ns3::NS3AIRL<EnvStruct, ActStruct>::set_act)
        .def("is_finished", &ns3::NS3AIRL<EnvStruct, ActStruct>::is_finished);

}


