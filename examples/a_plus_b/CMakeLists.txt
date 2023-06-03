build_lib_example(
        NAME ns3ai_apb
        SOURCE_FILES apb.cc
        LIBRARIES_TO_LINK ${libns3-ai}
)

pybind11_add_module(ns3ai_apb_py apb_py.cc)
set_target_properties(ns3ai_apb_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Build Python interface along with C++ lib
add_dependencies(ns3ai_apb ns3ai_apb_py)

