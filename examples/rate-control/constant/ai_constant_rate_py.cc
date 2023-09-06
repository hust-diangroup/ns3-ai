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

#include "ai-constant-rate-wifi-manager.h"

#include <ns3/ai-module.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_ratecontrol_constant_py, m)
{
    py::class_<ns3::AiConstantRateEnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("transmitStreams", &ns3::AiConstantRateEnvStruct::transmitStreams)
        .def_readwrite("supportedStreams", &ns3::AiConstantRateEnvStruct::supportedStreams)
        .def_readwrite("mcs", &ns3::AiConstantRateEnvStruct::mcs);

    py::class_<ns3::AiConstantRateActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("nss", &ns3::AiConstantRateActStruct::nss)
        .def_readwrite("next_mcs", &ns3::AiConstantRateActStruct::next_mcs);

    py::class_<
        ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct, ns3::AiConstantRateActStruct>>(
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
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                         ns3::AiConstantRateActStruct>::PyRecvBegin)
        .def("PyRecvEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                         ns3::AiConstantRateActStruct>::PyRecvEnd)
        .def("PySendBegin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                         ns3::AiConstantRateActStruct>::PySendBegin)
        .def("PySendEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                         ns3::AiConstantRateActStruct>::PySendEnd)
        .def("PyGetFinished",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                         ns3::AiConstantRateActStruct>::PyGetFinished)
        .def("GetCpp2PyStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                         ns3::AiConstantRateActStruct>::GetCpp2PyStruct,
             py::return_value_policy::reference)
        .def("GetPy2CppStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::AiConstantRateEnvStruct,
                                         ns3::AiConstantRateActStruct>::GetPy2CppStruct,
             py::return_value_policy::reference);
}
