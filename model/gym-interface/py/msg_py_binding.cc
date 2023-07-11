#include <pybind11/pybind11.h>
#include <ns3/ns3-ai-module.h>

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_gym_msg_py, m) {

    m.attr("msg_buffer_size") = MSG_BUFFER_SIZE;

    py::class_<Ns3AiGymMsg>(m, "Ns3AiGymMsg")
        .def(py::init<>())
        .def_readwrite("size", &Ns3AiGymMsg::size)
        .def("get_buffer", [](Ns3AiGymMsg &msg){
            // Get memoryview of the buffer
            return py::memoryview::from_memory(
                    (void *)msg.buffer,
                    msg.size
                );
        })
        ;

    py::class_<ns3::Ns3AiMsgInterface<Ns3AiGymMsg, Ns3AiGymMsg>>(m, "Ns3AiMsgInterface")
        .def(py::init<bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("py_recv_begin",
             &ns3::Ns3AiMsgInterface<Ns3AiGymMsg,
                                     Ns3AiGymMsg>::py_recv_begin)
        .def("py_recv_end",
             &ns3::Ns3AiMsgInterface<Ns3AiGymMsg,
                                     Ns3AiGymMsg>::py_recv_end)
        .def("py_send_begin",
             &ns3::Ns3AiMsgInterface<Ns3AiGymMsg,
                                     Ns3AiGymMsg>::py_send_begin)
        .def("py_send_end",
             &ns3::Ns3AiMsgInterface<Ns3AiGymMsg,
                                     Ns3AiGymMsg>::py_send_end)
        .def("py_check_finished",
             &ns3::Ns3AiMsgInterface<Ns3AiGymMsg,
                                     Ns3AiGymMsg>::py_check_finished)
        .def_readwrite("m_single_cpp2py_msg", &ns3::Ns3AiMsgInterface<Ns3AiGymMsg, Ns3AiGymMsg>::m_single_cpp2py_msg)
        .def_readwrite("m_single_py2cpp_msg", &ns3::Ns3AiMsgInterface<Ns3AiGymMsg, Ns3AiGymMsg>::m_single_py2cpp_msg)
        ;

}

