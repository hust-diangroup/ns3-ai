build_lib_example(
        NAME ns3ai_rltcp_gym
        SOURCE_FILES
            use_gym/sim.cc
            use_gym/tcp-rl-env.cc
            use_gym/tcp-rl.cc
        LIBRARIES_TO_LINK
            ${libns3-ai}
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
            use_msg/rl-tcp.cc
            use_msg/tcp-rl.cc
            use_msg/tcp-rl-env.cc
        LIBRARIES_TO_LINK
            ${libns3-ai}
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

pybind11_add_module(ns3ai_rltcp_msg_py use_msg/rl_tcp_py.cc)
set_target_properties(ns3ai_rltcp_msg_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use_msg)
target_link_libraries(ns3ai_rltcp_msg_py PRIVATE ${libns3-ai})

# Build Python interface along with C++ lib
add_dependencies(ns3ai_rltcp_msg ns3ai_rltcp_msg_py)
