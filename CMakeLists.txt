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
        LIBNAME ai
        SOURCE_FILES ${msg_interface_srcs} ${gym_interface_srcs}
        HEADER_FILES ${msg_interface_hdrs} ${gym_interface_hdrs}
        LIBRARIES_TO_LINK ${libcore} Boost::program_options protobuf::libprotobuf
)

# protobuf_generate function is missing in some installations by package manager
check_function_exists(protobuf_generate protobuf_generate_exists)
if(${protobuf_generate_exists})
    message(STATUS "protobuf_generate function found")
else()
    message(STATUS "protobuf_generate function not found -> use a local copy from ${CMAKE_CURRENT_SOURCE_DIR}/protobuf-generate.cmake")
    include(${CMAKE_CURRENT_SOURCE_DIR}/protobuf-generate.cmake)
endif()

# Compile proto files
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
add_dependencies(${libai} proto-objects)

# Build Gym msg binding module
add_subdirectory(model/gym-interface/py)

# Check if C++-based ML frameworks are available
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/model/libtensorflow)
    message(STATUS "TensorFlow C library found, examples using libtensorflow are enabled")
    set(NS3AI_LIBTENSORFLOW_EXAMPLES ON)
else()
    message(STATUS "TensorFlow C library not found, examples using libtensorflow are disabled")
    set(NS3AI_LIBTENSORFLOW_EXAMPLES OFF)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/model/libtorch)
    message(STATUS "PyTorch C++ library found, examples using libtorch are enabled")
    set(NS3AI_LIBTORCH_EXAMPLES ON)
else()
    message(STATUS "PyTorch C++ library not found, examples using libtorch are disabled")
    set(NS3AI_LIBTORCH_EXAMPLES OFF)
endif()
