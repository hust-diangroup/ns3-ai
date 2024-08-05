build_lib_example(
        NAME ns3ai_multi-agent
        SOURCE_FILES multi-agent.cc
        LIBRARIES_TO_LINK ${libai} ${libcore}
)
