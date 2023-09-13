# workaround for library outside /src
# If placed in /src, this block is unnecessary
set(libname vr-app)
library_target_name(${libname} targetname)
mark_as_advanced(lib${libname} lib${libname}-obj)
set(lib${libname} ${targetname} CACHE INTERNAL "")
set(lib${libname}-obj ${targetname}-obj CACHE INTERNAL "")
set(ns3-all-enabled-modules "${libname};${ns3-all-enabled-modules}"
        CACHE INTERNAL "list with all enabled modules"
)
#message("ns3-all-enabled-modules is ${ns3-all-enabled-modules}")

build_lib(
    LIBNAME vr-app
    SOURCE_FILES model/burst-generator.cc
                 model/burst-sink.cc
                 model/bursty-application.cc
                 model/my-random-variable-stream.cc
                 model/seq-ts-size-frag-header.cc
                 model/simple-burst-generator.cc
                 model/trace-file-burst-generator.cc
                 model/vr-burst-generator.cc
                 helper/bursty-helper.cc
                 helper/burst-sink-helper.cc
                 helper/bursty-app-stats-calculator.cc
    HEADER_FILES model/burst-generator.h
                 model/burst-sink.h
                 model/bursty-application.h
                 model/my-random-variable-stream.h
                 model/seq-ts-size-frag-header.h
                 model/simple-burst-generator.h
                 model/trace-file-burst-generator.h
                 model/vr-burst-generator.h
                 helper/bursty-helper.h
                 helper/burst-sink-helper.h
                 helper/bursty-app-stats-calculator.h
    LIBRARIES_TO_LINK ${libcore}
                        ${libapplications}
                        ${libpoint-to-point}
                        ${libinternet}
)
