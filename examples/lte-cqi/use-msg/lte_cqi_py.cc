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

#include "cqi-dl-env.h"

#include <ns3/ai-module.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_ltecqi_py, m)
{
    py::class_<ns3::CqiFeature>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("wbCqi", &ns3::CqiFeature::wbCqi);

    py::class_<ns3::CqiPredicted>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("new_wbCqi", &ns3::CqiPredicted::new_wbCqi);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>>(
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
             &ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>::PyRecvBegin)
        .def("PyRecvEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>::PyRecvEnd)
        .def("PySendBegin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>::PySendBegin)
        .def("PySendEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>::PySendEnd)
        .def("PyGetFinished",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>::PyGetFinished)
        .def("GetCpp2PyStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>::GetCpp2PyStruct,
             py::return_value_policy::reference)
        .def("GetPy2CppStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::CqiFeature, ns3::CqiPredicted>::GetPy2CppStruct,
             py::return_value_policy::reference);
}
