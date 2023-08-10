build_lib_example(
        NAME ns3ai_apb_msg_vec
        SOURCE_FILES use_msg_vec/apb.cc
        LIBRARIES_TO_LINK ${libns3-ai}
)
pybind11_add_module(ns3ai_apb_py use_msg_vec/apb_py.cc)
set_target_properties(ns3ai_apb_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use_msg_vec)

build_lib_example(
        NAME ns3ai_apb_msg_stru
        SOURCE_FILES use_msg_stru/apb.cc
        LIBRARIES_TO_LINK ${libns3-ai}
)
pybind11_add_module(ns3ai_apb_py_not_vec use_msg_stru/apb_py.cc)
set_target_properties(ns3ai_apb_py_not_vec PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use_msg_stru)

# Build Python binding library along with C++ library
add_dependencies(ns3ai_apb_msg_vec ns3ai_apb_py ns3ai_apb_py_not_vec)

build_lib_example(
        NAME ns3ai_apb_gym
        SOURCE_FILES use_gym/apb.cc
        LIBRARIES_TO_LINK ${libns3-ai} ${libcore}
)
