build_lib_example(
        NAME ns3ai_apb_msg_vec
        SOURCE_FILES use-msg-vec/apb.cc
        LIBRARIES_TO_LINK ${libai}
)
pybind11_add_module(ns3ai_apb_py_vec use-msg-vec/apb_py.cc)
set_target_properties(ns3ai_apb_py_vec PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use-msg-vec)

build_lib_example(
        NAME ns3ai_apb_msg_stru
        SOURCE_FILES use-msg-stru/apb.cc
        LIBRARIES_TO_LINK ${libai}
)
pybind11_add_module(ns3ai_apb_py_stru use-msg-stru/apb_py.cc)
set_target_properties(ns3ai_apb_py_stru PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/use-msg-stru)

# Build Python binding library along with C++ library
add_dependencies(ns3ai_apb_msg_vec ns3ai_apb_py_vec)
add_dependencies(ns3ai_apb_msg_stru ns3ai_apb_py_stru)

build_lib_example(
        NAME ns3ai_apb_gym
        SOURCE_FILES use-gym/apb.cc
        LIBRARIES_TO_LINK ${libai} ${libcore}
)
