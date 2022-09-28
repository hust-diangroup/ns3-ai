build_lib(
  LIBNAME ns3-ai
  SOURCE_FILES
    model/memory-pool.cc
  HEADER_FILES
    model/memory-pool.h
    model/train-var.h
    model/ns3-ai-rl.h
    model/ns3-ai-dl.h
  LIBRARIES_TO_LINK ${libcore}
)
