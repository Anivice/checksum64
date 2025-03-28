cmake_minimum_required(VERSION 3.10)
project(checksum64 C CXX)

# Enable Ccache support
find_program(CCACHE_PROGRAM ccache REQUIRED)
if(NOT CCACHE_PROGRAM)
    message(FATAL_ERROR "ccache not found! Please install ccache or adjust the configuration.")
endif()

# Set ccache as the compiler launcher before setting the compiler
set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")

# Set C and C++ standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable position-independent code if needed
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set visibility properties
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_definitions(__DEBUG__)
else ()
    add_compile_options(${optimization_flags})
    add_compile_options(${optimization_link_flags})
endif ()

add_library(log OBJECT src/log.cpp src/include/log.hpp)
add_library(libbin2hex OBJECT src/bin2hex.cpp src/include/bin2hex.h)

include_directories(src/include)
add_executable(ch64sum
        src/ch64sum.cpp
        src/argument_parser.cpp src/include/argument_parser.h
)
target_link_libraries(ch64sum PRIVATE log libbin2hex)
