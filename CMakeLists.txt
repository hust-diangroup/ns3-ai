set(name ns3-ai)

set(source_files
        model/memory-pool.cc
        )

set(header_files
        model/memory-pool.h
        model/train-var.h
        model/ns3-ai-rl.h
        model/ns3-ai-dl.h
        )

set(libraries_to_link
        libcore
        )

build_lib(
        LIBNAME ${name}
        SOURCE_FILES ${source_files}
        HEADER_FILES ${header_files}
        LIBRARIES_TO_LINK ${libraries_to_link}
)

