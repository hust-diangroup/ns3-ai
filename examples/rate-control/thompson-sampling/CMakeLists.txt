
# Python module for TS
pybind11_add_module(ns3ai_ratecontrol_ts_py ai_thompson_sampling_py.cc)
set_target_properties(ns3ai_ratecontrol_ts_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ns3ai_ratecontrol_ts_py PRIVATE
        ${libai}
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
add_dependencies(ns3ai_ratecontrol_ts ns3ai_ratecontrol_ts_py)

