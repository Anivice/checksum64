#  CMakeLists.txt
#
#  Copyright 2025 Anivice Ives
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
# 
#  SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.10)
project(checksum64 C CXX)

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
endif ()

add_compile_definitions(CRC64_VERSION="1.0.1")

add_library(log OBJECT src/log.cpp src/include/log.hpp)
add_library(libbin2hex OBJECT src/bin2hex.cpp src/include/bin2hex.h)

include_directories(src/include)
add_executable(crc64sum
        src/ch64sum.cpp
        src/argument_parser.cpp src/include/argument_parser.h
)
target_link_libraries(crc64sum PRIVATE log libbin2hex)
