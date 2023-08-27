build_lib_example(
        NAME ns3ai_ltecqi_msg
        SOURCE_FILES
            use_msg/lte_cqi.cc
            use_msg/cqi-dl-env.cc
            use_msg/my-rr-sched.cc
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

pybind11_add_module(ns3ai_ltecqi_py use_msg/lte_cqi_py.cc)
set_target_properties(ns3ai_ltecqi_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use_msg/)
target_link_libraries(ns3ai_ltecqi_py PRIVATE ${libai})

# Build Python interface along with C++ lib
add_dependencies(ns3ai_ltecqi_msg ns3ai_ltecqi_py)

