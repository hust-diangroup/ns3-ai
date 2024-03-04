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
    string(REPLACE "${PROJECT_SOURCE_DIR}/" "" FOLDER "${CMAKE_CURRENT_SOURCE_DIR}")
    build_exec(
        EXECNAME ns3ai_ltecqi_purecpp
        SOURCE_FILES pure-cpp/main.cc
        LIBRARIES_TO_LINK ${libcore} ${TensorFlow_LIBRARIES}
        EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FOLDER}/
    )
    target_include_directories(ns3ai_ltecqi_purecpp PRIVATE ${Libtensorflow_INCLUDE_DIR})
else()
    message(STATUS "LTE-CQI pure C++ example disabled")
endif()
