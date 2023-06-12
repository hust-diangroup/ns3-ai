build_lib_example(
        NAME ns3ai_ratecontrol
        SOURCE_FILES
            rate-control.cc
#            constant/ai-constant-rate-wifi-manager.cc
            thompson-sampling/ai-thompson-sampling-wifi-manager.cc
        LIBRARIES_TO_LINK
            ${libns3-ai}
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

#add_subdirectory(constant)
add_subdirectory(thompson-sampling)
