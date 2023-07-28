#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include <ns3/ns3-ai-module.h>
#include "ai-thompson-sampling-wifi-manager.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::array<ns3::ThompsonSamplingRateStats, 64>);

PYBIND11_MODULE(ns3ai_ratecontrol_ts_py, m) {

    py::class_<ns3::ThompsonSamplingRateStats>(m, "ThompsonSamplingRateStats")
        .def(py::init<>())
        .def_readwrite("nss", &ns3::ThompsonSamplingRateStats::nss)
        .def_readwrite("channelWidth", &ns3::ThompsonSamplingRateStats::channelWidth)
        .def_readwrite("guardInterval", &ns3::ThompsonSamplingRateStats::guardInterval)
        .def_readwrite("dataRate", &ns3::ThompsonSamplingRateStats::dataRate)
        .def_readwrite("success", &ns3::ThompsonSamplingRateStats::success)
        .def_readwrite("fails", &ns3::ThompsonSamplingRateStats::fails)
        .def_readwrite("lastDecay", &ns3::ThompsonSamplingRateStats::lastDecay)
        .def("__copy__", [](const ns3::ThompsonSamplingRateStats &self){
            return ns3::ThompsonSamplingRateStats(self);
        })
        ;

    py::class_<std::array<ns3::ThompsonSamplingRateStats, 64>>(m, "ThompsonSamplingRateStatsArray")
        .def(py::init<>())
        .def("size", &std::array<ns3::ThompsonSamplingRateStats, 64>::size)
        .def("__len__", [](const std::array<ns3::ThompsonSamplingRateStats, 64> &arr) {
            return arr.size();
        })
        .def("__getitem__", [](const std::array<ns3::ThompsonSamplingRateStats, 64> &arr, uint32_t i){
            if (i >= arr.size()) {
                std::cerr << "Invalid index " << i << " for std::array, whose size is " << arr.size() << std::endl;
                exit(1);
            }
            return arr.at(i);
        })
        ;

    py::class_<ns3::ThompsonSamplingEnvDecay>(m, "ThompsonSamplingEnvDecay")
        .def(py::init<>())
        .def_readwrite("decayIdx", &ns3::ThompsonSamplingEnvDecay::decayIdx)
        .def_readwrite("decay", &ns3::ThompsonSamplingEnvDecay::decay)
        .def_readwrite("now", &ns3::ThompsonSamplingEnvDecay::now)
        ;

    py::class_<ns3::ThompsonSamplingEnvPayloadStruct>(m, "ThompsonSamplingEnvPayloadStruct")
        .def(py::init<>())
        .def_readwrite("stats", &ns3::ThompsonSamplingEnvPayloadStruct::stats)
        .def_readwrite("decay", &ns3::ThompsonSamplingEnvPayloadStruct::decay)
        ;

    py::class_<ns3::AiThompsonSamplingEnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("type", &ns3::AiThompsonSamplingEnvStruct::type)
        .def_readwrite("managerId", &ns3::AiThompsonSamplingEnvStruct::managerId)
        .def_readwrite("stationId", &ns3::AiThompsonSamplingEnvStruct::stationId)
        .def_readwrite("var", &ns3::AiThompsonSamplingEnvStruct::var)
        .def_readwrite("data", &ns3::AiThompsonSamplingEnvStruct::data)
        ;

    py::class_<ns3::AiThompsonSamplingActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("managerId", &ns3::AiThompsonSamplingActStruct::managerId)
        .def_readwrite("stationId", &ns3::AiThompsonSamplingActStruct::stationId)
        .def_readwrite("res", &ns3::AiThompsonSamplingActStruct::res)
        .def_readwrite("stats", &ns3::AiThompsonSamplingActStruct::stats)
        ;

    py::class_<ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct, ns3::AiThompsonSamplingActStruct>>(m, "Ns3AiMsgInterface")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin",
             &ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct,
                                     ns3::AiThompsonSamplingActStruct>::py_recv_begin)
        .def("py_recv_end",
             &ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct,
                                     ns3::AiThompsonSamplingActStruct>::py_recv_end)
        .def("py_send_begin",
             &ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct,
                                     ns3::AiThompsonSamplingActStruct>::py_send_begin)
        .def("py_send_end",
             &ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct,
                                     ns3::AiThompsonSamplingActStruct>::py_send_end)
        .def("py_get_finished",
             &ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct,
                                     ns3::AiThompsonSamplingActStruct>::py_get_finished)
        .def_readwrite("m_single_cpp2py_msg", &ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct, ns3::AiThompsonSamplingActStruct>::m_single_cpp2py_msg)
        .def_readwrite("m_single_py2cpp_msg", &ns3::Ns3AiMsgInterface<ns3::AiThompsonSamplingEnvStruct, ns3::AiThompsonSamplingActStruct>::m_single_py2cpp_msg)
        ;

}

