build_lib_example(
        NAME ns3ai_ltecqi
        SOURCE_FILES
            lte_cqi.cc
            cqi-dl-env.cc
            my-rr-sched.cc
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

pybind11_add_module(ns3ai_ltecqi_py lte_cqi_py.cc)
set_target_properties(ns3ai_ltecqi_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ns3ai_ltecqi_py PRIVATE ${libai})

# Build Python interface along with C++ lib
add_dependencies(ns3ai_ltecqi ns3ai_ltecqi_py)

