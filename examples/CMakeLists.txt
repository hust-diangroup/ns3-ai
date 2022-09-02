build_lib_example(
    NAME ai-rl-tcp
    SOURCE_FILES
        rl-tcp/sim.cc
        rl-tcp/tcp-rl.cc
        rl-tcp/tcp-rl-env.cc
    LIBRARIES_TO_LINK
        ${libapplications}
        ${libcore}
        ${libflow-monitor}
        ${libinternet}
        ${libns3-ai}
        ${libpoint-to-point-layout}
        ${libptp}

)