check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Wno-gcc-compat")
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Python REQUIRED COMPONENTS Interpreter Development)
if (Python_FOUND)
    message(STATUS "Python include directories: ${Python_INCLUDE_DIRS}")
    message(STATUS "Python library directories: ${Python_LIBRARY_DIRS}")
endif()
find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})

# we need CONFIG for debug mode in macOS
find_package(Protobuf CONFIG)
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()
include_directories(${Protobuf_INCLUDE_DIRS})

# Check if C++-based ML frameworks are available
# Must do before build_lib (which adds examples/ subdirectory)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/model/libtensorflow)
    message(STATUS "TensorFlow C library found, examples using libtensorflow are enabled")
    set(NS3AI_LIBTENSORFLOW_EXAMPLES ON)
    set(Libtensorflow_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model/libtensorflow/include")
    if(NOT EXISTS ${Libtensorflow_INCLUDE_DIR})
        message(FATAL_ERROR "Include directory of TensorFlow C library does not exist")
    endif()
    set(Libtensorflow_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model/libtensorflow/lib")
    if(NOT EXISTS ${Libtensorflow_LIBRARY_DIR})
        message(FATAL_ERROR "Library directory of TensorFlow C library does not exist")
    else()
        file(GLOB TensorFlow_LIBRARIES "${Libtensorflow_LIBRARY_DIR}/*.so" "${Libtensorflow_LIBRARY_DIR}/*.dylib")
    endif()
else()
    message(STATUS "TensorFlow C library not found, examples using libtensorflow are disabled")
    set(NS3AI_LIBTENSORFLOW_EXAMPLES OFF)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/model/libtorch)
    message(STATUS "PyTorch C++ library found, examples using libtorch are enabled")
    set(NS3AI_LIBTORCH_EXAMPLES ON)
    set(Libtorch_INCLUDE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/model/libtorch/include;${CMAKE_CURRENT_SOURCE_DIR}/model/libtorch/include/torch/csrc/api/include")
    foreach(dir ${Libtorch_INCLUDE_DIRS})
        if(NOT EXISTS ${dir})
            message(FATAL_ERROR "Include directory ${dir} of PyTorch C++ library does not exist")
        endif()
    endforeach ()
    set(Libtorch_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model/libtorch/lib")
    if(NOT EXISTS ${Libtorch_LIBRARY_DIR})
        message(FATAL_ERROR "Library directory of PyTorch C++ library does not exist")
    else()
        file(GLOB Torch_LIBRARIES "${Libtorch_LIBRARY_DIR}/*.so" "${Libtorch_LIBRARY_DIR}/*.dylib")
    endif()
else()
    message(STATUS "PyTorch C++ library not found, examples using libtorch are disabled")
    set(NS3AI_LIBTORCH_EXAMPLES OFF)
endif()

set(msg_interface_srcs )
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

build_lib(
        LIBNAME ai
        SOURCE_FILES ${msg_interface_srcs} ${gym_interface_srcs}
        HEADER_FILES ${msg_interface_hdrs} ${gym_interface_hdrs}
        LIBRARIES_TO_LINK ${libcore} protobuf::libprotobuf
)
add_dependencies(${libai} proto-objects)

# Build Gym msg binding module
add_subdirectory(model/gym-interface/py)
