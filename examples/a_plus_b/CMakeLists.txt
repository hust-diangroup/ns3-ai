build_lib_example(
        NAME ns3ai_apb_msg
        SOURCE_FILES use_msg/apb.cc
        LIBRARIES_TO_LINK ${libns3-ai}
)

pybind11_add_module(ns3ai_apb_py use_msg/apb_py.cc)
set_target_properties(ns3ai_apb_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use_msg)

# Build Python binding library along with C++ library
add_dependencies(ns3ai_apb_msg ns3ai_apb_py)


build_lib_example(
        NAME ns3ai_apb_gym
        SOURCE_FILES use_gym/apb.cc
        LIBRARIES_TO_LINK ${libns3-ai} ${libcore}
)
