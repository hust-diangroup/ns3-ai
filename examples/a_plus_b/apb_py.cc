#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include <ns3/ns3-ai-rl.h>
#include "apb.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemEnvVector);
PYBIND11_MAKE_OPAQUE(ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemActVector);

PYBIND11_MODULE(ns3ai_apb_py, m) {

    py::class_<EnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("a", &EnvStruct::env_a)
        .def_readwrite("b", &EnvStruct::env_b);

    py::class_<ActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("c", &ActStruct::act_c);

    py::class_<ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemEnvVector>(m, "PyEnvVector")
        .def("resize", static_cast
             <void (ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemEnvVector::*)
                  (ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemEnvVector::size_type)>
             (&ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemEnvVector::resize))
        .def("__len__", &ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemEnvVector::size)
        .def("__getitem__", [](ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemEnvVector &vec, int i) -> EnvStruct & {
            if (i < 0 || i >= vec.size()) {
                std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                exit(1);
            }
            return vec.at(i);
        }, py::return_value_policy::reference)
        ;

    py::class_<ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemActVector>(m, "PyActVector")
        .def("resize", static_cast
             <void (ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemActVector::*)
                  (ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemActVector::size_type)>
             (&ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemActVector::resize))
        .def("__len__", &ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemActVector::size)
        .def("__getitem__", [](ns3::Ns3AiRl<EnvStruct, ActStruct>::ShmemActVector &vec, int i) -> ActStruct & {
            if (i < 0 || i >= vec.size()) {
                std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                exit(1);
            }
            return vec.at(i);
        }, py::return_value_policy::reference)
        ;

    py::class_<ns3::Ns3AiRl<EnvStruct, ActStruct>>(m, "Ns3AiRl")
        .def(py::init<uint32_t, bool, bool, const char*, const char*, const char*, const char*>())
        .def("get_env_begin", &ns3::Ns3AiRl<EnvStruct, ActStruct>::get_env_begin)
        .def("get_env_end", &ns3::Ns3AiRl<EnvStruct, ActStruct>::get_env_end)
        .def("set_act_begin", &ns3::Ns3AiRl<EnvStruct, ActStruct>::set_act_begin)
        .def("set_act_end", &ns3::Ns3AiRl<EnvStruct, ActStruct>::set_act_end)
        .def("is_finished", &ns3::Ns3AiRl<EnvStruct, ActStruct>::is_finished)
        .def_readwrite("m_env", &ns3::Ns3AiRl<EnvStruct, ActStruct>::m_env)
        .def_readwrite("m_act", &ns3::Ns3AiRl<EnvStruct, ActStruct>::m_act)
        ;

}


