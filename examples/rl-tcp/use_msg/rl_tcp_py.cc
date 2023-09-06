#include "tcp-rl-env.h"

#include <ns3/ai-module.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_rltcp_msg_py, m)
{
    py::class_<ns3::TcpRlEnv>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("nodeId", &ns3::TcpRlEnv::nodeId)
        .def_readwrite("socketUid", &ns3::TcpRlEnv::socketUid)
        .def_readwrite("envType", &ns3::TcpRlEnv::envType)
        .def_readwrite("simTime_us", &ns3::TcpRlEnv::simTime_us)
        .def_readwrite("ssThresh", &ns3::TcpRlEnv::ssThresh)
        .def_readwrite("cWnd", &ns3::TcpRlEnv::cWnd)
        .def_readwrite("segmentSize", &ns3::TcpRlEnv::segmentSize)
        .def_readwrite("segmentsAcked", &ns3::TcpRlEnv::segmentsAcked)
        .def_readwrite("bytesInFlight", &ns3::TcpRlEnv::bytesInFlight);

    py::class_<ns3::TcpRlAct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("new_ssThresh", &ns3::TcpRlAct::new_ssThresh)
        .def_readwrite("new_cWnd", &ns3::TcpRlAct::new_cWnd);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool,
                      bool,
                      bool,
                      uint32_t,
                      const char*,
                      const char*,
                      const char*,
                      const char*>())
        .def("PyRecvBegin", &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::PyRecvBegin)
        .def("PyRecvEnd", &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::PyRecvEnd)
        .def("PySendBegin", &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::PySendBegin)
        .def("PySendEnd", &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::PySendEnd)
        .def("PyGetFinished",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::PyGetFinished)
        .def("GetCpp2PyStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::GetCpp2PyStruct,
             py::return_value_policy::reference)
        .def("GetPy2CppStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::GetPy2CppStruct,
             py::return_value_policy::reference);
}
