set(source_files
    a_plus_b.cc
)

build_lib_example(
  NAME a_plus_b
  SOURCE_FILES ${source_files}
  LIBRARIES_TO_LINK
    ${libcore}
    ${libns3-ai}
)
