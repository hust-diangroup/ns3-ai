#include <pybind11/pybind11.h>

#include <ns3/ns3-ai-module.h>
#include "cqi-dl-env.h"

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_ltecqi_py, m) {

    py::class_<ns3::CqiFeature>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("wbCqi", &ns3::CqiFeature::wbCqi)
        ;

    py::class_<ns3::CqiPredicted>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("new_wbCqi", &ns3::CqiPredicted::new_wbCqi)
        ;

    py::class_<ns3::Ns3AiMsgInterface<ns3::CqiFeature, ns3::CqiPredicted>>(m, "Ns3AiMsgInterface")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin",
             &ns3::Ns3AiMsgInterface<ns3::CqiFeature,
                                     ns3::CqiPredicted>::py_recv_begin)
        .def("py_recv_end",
             &ns3::Ns3AiMsgInterface<ns3::CqiFeature,
                                     ns3::CqiPredicted>::py_recv_end)
        .def("py_send_begin",
             &ns3::Ns3AiMsgInterface<ns3::CqiFeature,
                                     ns3::CqiPredicted>::py_send_begin)
        .def("py_send_end",
             &ns3::Ns3AiMsgInterface<ns3::CqiFeature,
                                     ns3::CqiPredicted>::py_send_end)
        .def("py_get_finished",
             &ns3::Ns3AiMsgInterface<ns3::CqiFeature,
                                     ns3::CqiPredicted>::py_get_finished)
        .def_readwrite("m_single_cpp2py_msg", &ns3::Ns3AiMsgInterface<ns3::CqiFeature, ns3::CqiPredicted>::m_single_cpp2py_msg)
        .def_readwrite("m_single_py2cpp_msg", &ns3::Ns3AiMsgInterface<ns3::CqiFeature, ns3::CqiPredicted>::m_single_py2cpp_msg)
        ;

}

