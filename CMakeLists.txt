check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_package(pybind11 REQUIRED)

build_lib(
        LIBNAME ns3-ai
        SOURCE_FILES model/ns3-ai-new-rl.cc
        HEADER_FILES model/ns3-ai-new-rl.h
        LIBRARIES_TO_LINK ${libcore} Boost::program_options
)


