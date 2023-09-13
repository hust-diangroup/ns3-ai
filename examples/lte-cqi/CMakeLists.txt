build_lib_example(
        NAME ns3ai_ltecqi_msg
        SOURCE_FILES
            use-msg/lte_cqi.cc
            use-msg/cqi-dl-env.cc
            use-msg/my-rr-sched.cc
        LIBRARIES_TO_LINK
            ${libai}
            ${libcore}
            ${libpoint-to-point}
            ${libnetwork}
            ${libapplications}
            ${libmobility}
            ${libcsma}
            ${libinternet}
            ${libflow-monitor}
            ${liblte}
)

pybind11_add_module(ns3ai_ltecqi_py use-msg/lte_cqi_py.cc)
set_target_properties(ns3ai_ltecqi_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use-msg/)
target_link_libraries(ns3ai_ltecqi_py PRIVATE ${libai})

# Build Python interface along with C++ lib
add_dependencies(ns3ai_ltecqi_msg ns3ai_ltecqi_py)

# Check if libtensorflow exists, if true, enable the pure C++ example
if(NS3AI_LIBTENSORFLOW_EXAMPLES)
    message(STATUS "LTE-CQI pure C++ example enabled")
    build_lib_example(
            NAME ns3ai_ltecqi_purecpp
            SOURCE_FILES pure-cpp/main.cc
            LIBRARIES_TO_LINK   # Need not link with ns3-ai because there is no IPC between Python and C++
                ${libcore}
                ${TensorFlow_LIBRARIES}
    )
    target_include_directories(ns3ai_ltecqi_purecpp PRIVATE ${Libtensorflow_INCLUDE_DIR})
else()
    message(STATUS "LTE-CQI pure C++ example disabled")
endif()
