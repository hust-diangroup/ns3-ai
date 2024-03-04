build_lib_example(
        NAME ns3ai_rltcp_gym
        SOURCE_FILES
            use-gym/sim.cc
            use-gym/tcp-rl-env.cc
            use-gym/tcp-rl.cc
        LIBRARIES_TO_LINK
            ${libai}
            ${libcore}
            ${libpoint-to-point}
            ${libpoint-to-point-layout}
            ${libnetwork}
            ${libapplications}
            ${libmobility}
            ${libcsma}
            ${libinternet}
            ${libwifi}
            ${libflow-monitor}
)

build_lib_example(
        NAME ns3ai_rltcp_msg
        SOURCE_FILES
            use-msg/rl-tcp.cc
            use-msg/tcp-rl.cc
            use-msg/tcp-rl-env.cc
        LIBRARIES_TO_LINK
            ${libai}
            ${libcore}
            ${libpoint-to-point}
            ${libpoint-to-point-layout}
            ${libnetwork}
            ${libapplications}
            ${libmobility}
            ${libcsma}
            ${libinternet}
            ${libwifi}
            ${libflow-monitor}
)

pybind11_add_module(ns3ai_rltcp_msg_py use-msg/rl_tcp_py.cc)
set_target_properties(ns3ai_rltcp_msg_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use-msg)
target_link_libraries(ns3ai_rltcp_msg_py PRIVATE ${libai})

# Build Python interface along with C++ lib
add_dependencies(ns3ai_rltcp_msg ns3ai_rltcp_msg_py)

# Check if libtorch exists, if true, enable the pure C++ example
if(NS3AI_LIBTORCH_EXAMPLES)
    message(STATUS "RL-TCP pure C++ example enabled")
    build_lib_example(
            NAME ns3ai_rltcp_purecpp
            SOURCE_FILES pure-cpp/rl-tcp.cc
                         pure-cpp/tcp-rl.cc
                         pure-cpp/tcp-rl-env.cc
            LIBRARIES_TO_LINK
            ${libcore}
            ${Torch_LIBRARIES}
            ${Python_LIBRARIES}  # need to link with Python, otherwise symbol _PyBaseObject_Type will be missing
            ${libpoint-to-point}
            ${libpoint-to-point-layout}
            ${libnetwork}
            ${libapplications}
            ${libmobility}
            ${libcsma}
            ${libinternet}
            ${libwifi}
            ${libflow-monitor}
    )
    target_include_directories(ns3ai_rltcp_purecpp PRIVATE ${Libtorch_INCLUDE_DIRS})
else()
    message(STATUS "RL-TCP pure C++ example disabled")
endif()
