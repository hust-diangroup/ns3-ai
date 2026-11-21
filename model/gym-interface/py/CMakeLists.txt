ns3ai_add_pybind_module(ns3ai_gym_msg_py msg_py_binding.cc)
set_target_properties(ns3ai_gym_msg_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Build Python interface along with C++ lib
if(TARGET ${libai})
    add_dependencies(${libai} ns3ai_gym_msg_py)
elseif(TARGET ai)
    add_dependencies(ai ns3ai_gym_msg_py)
endif()
