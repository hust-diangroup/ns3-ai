#include <pybind11/pybind11.h>

#include <ns3/ai-module.h>
#include "tcp-rl-env.h"

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_rltcp_msg_py, m) {

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
        .def_readwrite("bytesInFlight", &ns3::TcpRlEnv::bytesInFlight)
        ;

    py::class_<ns3::TcpRlAct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("new_ssThresh", &ns3::TcpRlAct::new_ssThresh)
        .def_readwrite("new_cWnd", &ns3::TcpRlAct::new_cWnd)
        ;

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv,
                                     ns3::TcpRlAct>::py_recv_begin)
        .def("py_recv_end",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv,
                                     ns3::TcpRlAct>::py_recv_end)
        .def("py_send_begin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv,
                                     ns3::TcpRlAct>::py_send_begin)
        .def("py_send_end",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv,
                                     ns3::TcpRlAct>::py_send_end)
        .def("py_get_finished",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv,
                                     ns3::TcpRlAct>::py_get_finished)
        .def_readwrite("m_single_cpp2py_msg", &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::m_single_cpp2py_msg)
        .def_readwrite("m_single_py2cpp_msg", &ns3::Ns3AiMsgInterfaceImpl<ns3::TcpRlEnv, ns3::TcpRlAct>::m_single_py2cpp_msg)
        ;

}

