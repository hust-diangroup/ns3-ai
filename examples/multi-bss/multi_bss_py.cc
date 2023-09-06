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

#include "multi-bss.h"

#include <ns3/ai-module.h>

#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Cpp2PyMsgVector);
PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Py2CppMsgVector);

PYBIND11_MODULE(ns3ai_multibss_py, m)
{
    py::class_<std::array<double, 5>>(m, "RxPowerArray")
        .def(py::init<>())
        .def("size", &std::array<double, 5>::size)
        .def("__len__", [](const std::array<double, 5>& arr) { return arr.size(); })
        .def("__getitem__", [](const std::array<double, 5>& arr, uint32_t i) {
            if (i >= arr.size())
            {
                std::cerr << "Invalid index " << i << " for std::array, whose size is "
                          << arr.size() << std::endl;
                exit(1);
            }
            return arr.at(i);
        });

    py::class_<Env>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("txNode", &Env::txNode)
        .def_readwrite("rxPower", &Env::rxPower)
        .def_readwrite("mcs", &Env::mcs)
        .def_readwrite("holDelay", &Env::holDelay)
        .def_readwrite("throughput", &Env::throughput);

    py::class_<Act>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("newCcaSensitivity", &Act::newCcaSensitivity);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Cpp2PyMsgVector>(m, "PyEnvVector")
        .def("resize",
             static_cast<void (ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Cpp2PyMsgVector::*)(
                 ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Cpp2PyMsgVector::size_type)>(
                 &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Cpp2PyMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Cpp2PyMsgVector::size)
        .def(
            "__getitem__",
            [](ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Cpp2PyMsgVector& vec, uint32_t i) -> Env& {
                if (i >= vec.size())
                {
                    std::cerr << "Invalid index " << i << " for vector, whose size is "
                              << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            },
            py::return_value_policy::reference);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Py2CppMsgVector>(m, "PyActVector")
        .def("resize",
             static_cast<void (ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Py2CppMsgVector::*)(
                 ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Py2CppMsgVector::size_type)>(
                 &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Py2CppMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Py2CppMsgVector::size)
        .def(
            "__getitem__",
            [](ns3::Ns3AiMsgInterfaceImpl<Env, Act>::Py2CppMsgVector& vec, uint32_t i) -> Act& {
                if (i >= vec.size())
                {
                    std::cerr << "Invalid index " << i << " for vector, whose size is "
                              << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            },
            py::return_value_policy::reference);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<Env, Act>>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool,
                      bool,
                      bool,
                      uint32_t,
                      const char*,
                      const char*,
                      const char*,
                      const char*>())
        .def("PyRecvBegin", &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::PyRecvBegin)
        .def("PyRecvEnd", &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::PyRecvEnd)
        .def("PySendBegin", &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::PySendBegin)
        .def("PySendEnd", &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::PySendEnd)
        .def("PyGetFinished", &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::PyGetFinished)
        .def("GetCpp2PyVector",
             &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::GetCpp2PyVector,
             py::return_value_policy::reference)
        .def("GetPy2CppVector",
             &ns3::Ns3AiMsgInterfaceImpl<Env, Act>::GetPy2CppVector,
             py::return_value_policy::reference);
}
