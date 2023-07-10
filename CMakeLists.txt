check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})
find_package(Protobuf CONFIG REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

build_lib(
        LIBNAME ns3-ai
        SOURCE_FILES
            model/msg-interface/ns3-ai-msg-interface.cc
            model/gym-interface/messages.proto
            model/gym-interface/ns3-ai-gym-interface.cc
            model/gym-interface/ns3-ai-gym-env.cc
            model/gym-interface/container.cc
            model/gym-interface/spaces.cc
        HEADER_FILES
            model/msg-interface/ns3-ai-msg-interface.h
            model/gym-interface/ns3-ai-gym-interface.h
        LIBRARIES_TO_LINK ${libcore} Boost::program_options protobuf::libprotobuf
)

protobuf_generate(
        TARGET ${libns3-ai-obj}
        IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/model/gym-interface"
        LANGUAGE cpp
        PROTOC_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model/gym-interface"
)
