#include <ns3/ai-module.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_gym_msg_py, m)
{
    m.attr("msg_buffer_size") = MSG_BUFFER_SIZE;

    py::class_<Ns3AiGymMsg>(m, "Ns3AiGymMsg")
        .def(py::init<>())
        .def_readwrite("size", &Ns3AiGymMsg::size)
        .def("get_buffer",
             [](Ns3AiGymMsg& msg) {
                 // Get memoryview of the buffer
                 return py::memoryview::from_memory((void*)msg.buffer, msg.size);
             })
        .def("get_buffer_full", [](Ns3AiGymMsg& msg) {
            // Get memoryview of the buffer
            return py::memoryview::from_memory((void*)msg.buffer, MSG_BUFFER_SIZE);
        });

    py::class_<ns3::Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool,
                      bool,
                      bool,
                      uint32_t,
                      const char*,
                      const char*,
                      const char*,
                      const char*>())
        .def("PyRecvBegin", &ns3::Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>::PyRecvBegin)
        .def("PyRecvEnd", &ns3::Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>::PyRecvEnd)
        .def("PySendBegin", &ns3::Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>::PySendBegin)
        .def("PySendEnd", &ns3::Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>::PySendEnd)
        .def("GetCpp2PyStruct",
             &ns3::Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>::GetCpp2PyStruct,
             py::return_value_policy::reference)
        .def("GetPy2CppStruct",
             &ns3::Ns3AiMsgInterfaceImpl<Ns3AiGymMsg, Ns3AiGymMsg>::GetPy2CppStruct,
             py::return_value_policy::reference);
}
