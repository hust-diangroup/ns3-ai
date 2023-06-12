
# Python module for TS
pybind11_add_module(ns3ai_ratecontrol_ts_py ai_thompson_sampling_py.cc)
set_target_properties(ns3ai_ratecontrol_ts_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ns3ai_ratecontrol_ts_py PRIVATE
        ${libns3-ai}
        ${libcore}
        ${libpoint-to-point}
        ${libnetwork}
        ${libapplications}
        ${libmobility}
        ${libcsma}
        ${libinternet}
        ${libwifi}
        ${libflow-monitor})

# Build Python interface along with C++ lib
add_dependencies(ns3ai_ratecontrol ns3ai_ratecontrol_ts_py)

add_executable(aits_py_side aits_py_side.cc)
target_link_libraries(aits_py_side ${libcore} Boost::program_options)

