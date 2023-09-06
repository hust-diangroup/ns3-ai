/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#include "ai-thompson-sampling-wifi-manager.h"

#include <ns3/ai-module.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::array<ns3::ThompsonSamplingRateStats, 64>);

PYBIND11_MODULE(ns3ai_ratecontrol_ts_py, m)
{
    py::class_<ns3::ThompsonSamplingRateStats>(m, "ThompsonSamplingRateStats")
        .def(py::init<>())
        .def_readwrite("nss", &ns3::ThompsonSamplingRateStats::nss)
        .def_readwrite("channelWidth", &ns3::ThompsonSamplingRateStats::channelWidth)
        .def_readwrite("guardInterval", &ns3::ThompsonSamplingRateStats::guardInterval)
        .def_readwrite("dataRate", &ns3::ThompsonSamplingRateStats::dataRate)
        .def_readwrite("success", &ns3::ThompsonSamplingRateStats::success)
        .def_readwrite("fails", &ns3::ThompsonSamplingRateStats::fails)
        .def_readwrite("lastDecay", &ns3::ThompsonSamplingRateStats::lastDecay)
        .def("__copy__", [](const ns3::ThompsonSamplingRateStats& self) {
            return ns3::ThompsonSamplingRateStats(self);
        });

    py::class_<std::array<ns3::ThompsonSamplingRateStats, 64>>(m, "ThompsonSamplingRateStatsArray")
        .def(py::init<>())
        .def("size", &std::array<ns3::ThompsonSamplingRateStats, 64>::size)
        .def("__len__",
             [](const std::array<ns3::ThompsonSamplingRateStats, 64>& arr) { return arr.size(); })
        .def("__getitem__",
             [](const std::array<ns3::ThompsonSamplingRateStats, 64>& arr, uint32_t i) {
                 if (i >= arr.size())
                 {
                     std::cerr << "Invalid index " << i << " for std::array, whose size is "
                               << arr.size() << std::endl;
                     exit(1);
                 }
                 return arr.at(i);
             });

    py::class_<ns3::ThompsonSamplingEnvDecay>(m, "ThompsonSamplingEnvDecay")
        .def(py::init<>())
        .def_readwrite("decayIdx", &ns3::ThompsonSamplingEnvDecay::decayIdx)
        .def_readwrite("decay", &ns3::ThompsonSamplingEnvDecay::decay)
        .def_readwrite("now", &ns3::ThompsonSamplingEnvDecay::now);

    py::class_<ns3::ThompsonSamplingEnvPayloadStruct>(m, "ThompsonSamplingEnvPayloadStruct")
        .def(py::init<>())
        .def_readwrite("stats", &ns3::ThompsonSamplingEnvPayloadStruct::stats)
        .def_readwrite("decay", &ns3::ThompsonSamplingEnvPayloadStruct::decay);

    py::class_<ns3::AiThompsonSamplingEnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("type", &ns3::AiThompsonSamplingEnvStruct::type)
        .def_readwrite("managerId", &ns3::AiThompsonSamplingEnvStruct::managerId)
        .def_readwrite("stationId", &ns3::AiThompsonSamplingEnvStruct::stationId)
        .def_readwrite("var", &ns3::AiThompsonSamplingEnvStruct::var)
        .def_readwrite("data", &ns3::AiThompsonSamplingEnvStruct::data);

    py::class_<ns3::AiThompsonSamplingActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("managerId", &ns3::AiThompsonSamplingActStruct::managerId)
        .def_readwrite("stationId", &ns3::AiThompsonSamplingActStruct::stationId)
        .def_readwrite("res", &ns3::AiThompsonSamplingActStruct::res)
        .def_readwrite("stats", &ns3::AiThompsonSamplingActStruct::stats);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                          ns3::AiThompsonSamplingActStruct>>(
        m,
        "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool,
                      bool,
                      bool,
                      uint32_t,
                      const char*,
                      const char*,
                      const char*,
                      const char*>())
        .def("PyRecvBegin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                         ns3::AiThompsonSamplingActStruct>::PyRecvBegin)
        .def("PyRecvEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                         ns3::AiThompsonSamplingActStruct>::PyRecvEnd)
        .def("PySendBegin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                         ns3::AiThompsonSamplingActStruct>::PySendBegin)
        .def("PySendEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                         ns3::AiThompsonSamplingActStruct>::PySendEnd)
        .def("PyGetFinished",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                         ns3::AiThompsonSamplingActStruct>::PyGetFinished)
        .def("GetCpp2PyStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                         ns3::AiThompsonSamplingActStruct>::GetCpp2PyStruct,
             py::return_value_policy::reference)
        .def("GetPy2CppStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiThompsonSamplingEnvStruct,
                                         ns3::AiThompsonSamplingActStruct>::GetPy2CppStruct,
             py::return_value_policy::reference);
}
