check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})
find_package(Protobuf CONFIG REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

set(msg_interface_srcs model/msg-interface/ns3-ai-msg-interface.cc)
set(msg_interface_hdrs model/msg-interface/ns3-ai-msg-interface.h)
set(gym_interface_srcs
        model/gym-interface/cpp/ns3-ai-gym-interface.cc
        model/gym-interface/cpp/ns3-ai-gym-env.cc
        model/gym-interface/cpp/container.cc
        model/gym-interface/cpp/spaces.cc
        model/gym-interface/cpp/messages.pb.cc
)
set(gym_interface_hdrs
        model/gym-interface/cpp/ns3-ai-gym-interface.h
        model/gym-interface/cpp/ns3-ai-gym-env.h
        model/gym-interface/cpp/container.h
        model/gym-interface/cpp/spaces.h
)

build_lib(
        LIBNAME ns3-ai
        SOURCE_FILES ${msg_interface_srcs} ${gym_interface_srcs}
        HEADER_FILES ${msg_interface_hdrs} ${gym_interface_hdrs}
        LIBRARIES_TO_LINK ${libcore} Boost::program_options protobuf::libprotobuf
)

add_library(proto-objects OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/model/gym-interface/messages.proto")

protobuf_generate(
        TARGET proto-objects
        IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/model/gym-interface"
        LANGUAGE cpp
        PROTOC_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model/gym-interface/cpp"
)

protobuf_generate(
        TARGET proto-objects
        IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/model/gym-interface"
        LANGUAGE python
        PROTOC_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model/gym-interface/py"
)

add_dependencies(${libns3-ai} proto-objects)

add_subdirectory(model/gym-interface/py)
