pybind11_add_module(ns3ai_gym_msg_py msg_py_binding.cc)
set_target_properties(ns3ai_gym_msg_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Build Python interface along with C++ lib
add_dependencies(${libai} ns3ai_gym_msg_py)
