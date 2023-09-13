build_lib_example(
        NAME ns3ai_ratecontrol_constant
        SOURCE_FILES
            rate-control.cc
            constant/ai-constant-rate-wifi-manager.cc
        LIBRARIES_TO_LINK
            ${libai}
            ${libcore}
            ${libpoint-to-point}
            ${libnetwork}
            ${libapplications}
            ${libmobility}
            ${libcsma}
            ${libinternet}
            ${libwifi}
            ${libflow-monitor}
)

build_lib_example(
        NAME ns3ai_ratecontrol_ts
        SOURCE_FILES
        rate-control.cc
        thompson-sampling/ai-thompson-sampling-wifi-manager.cc
        LIBRARIES_TO_LINK
        ${libai}
        ${libcore}
        ${libpoint-to-point}
        ${libnetwork}
        ${libapplications}
        ${libmobility}
        ${libcsma}
        ${libinternet}
        ${libwifi}
        ${libflow-monitor}
)

add_subdirectory(constant)
add_subdirectory(thompson-sampling)
