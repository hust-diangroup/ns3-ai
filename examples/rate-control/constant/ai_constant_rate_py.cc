#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include <ns3/ns3-ai-new-rl.h>
#include "ai-constant-rate-wifi-manager.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(ns3::AiConstantRateEnvStruct);
PYBIND11_MAKE_OPAQUE(ns3::AiConstantRateActStruct);
PYBIND11_MAKE_OPAQUE(ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemEnvVector);
PYBIND11_MAKE_OPAQUE(ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemActVector);

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

    py::class_<ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemEnvVector>(m, "PyEnvVector")
        .def("resize", static_cast
             <void (ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemEnvVector::*)
                  (ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemEnvVector::size_type, const ns3::AiConstantRateEnvStruct &)>
             (&ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemEnvVector::resize))
        .def("__len__", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemEnvVector::size)
        .def("__getitem__", [](ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemEnvVector &vec, int i) -> ns3::AiConstantRateEnvStruct & {
                if (i < 0 || i >= vec.size()) {
                    std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            }, py::return_value_policy::reference)
        ;

    py::class_<ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemActVector>(m, "PyActVector")
        .def("resize", static_cast
             <void (ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemActVector::*)
                  (ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemActVector::size_type, const ns3::AiConstantRateActStruct &)>
             (&ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemActVector::resize))
        .def("__len__", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemActVector::size)
        .def("__getitem__", [](ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::ShmemActVector &vec, int i) -> ns3::AiConstantRateActStruct & {
                if (i < 0 || i >= vec.size()) {
                    std::cerr << "Invalid index " << i << " for vector, whose size is " << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            }, py::return_value_policy::reference)
        ;

    py::class_<ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>>(m, "NS3AIRL")
        .def(py::init<uint32_t, bool, const char*, const char*, const char*, const char*>())
        .def("get_env_begin", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::get_env_begin)
        .def("get_env_end", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::get_env_end)
        .def("set_act_begin", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::set_act_begin)
        .def("set_act_end", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::set_act_end)
        .def("is_finished", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::is_finished)
        .def_readwrite("m_env", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::m_env)
        .def_readwrite("m_act", &ns3::NS3AIRL<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>::m_act)
        ;

}

