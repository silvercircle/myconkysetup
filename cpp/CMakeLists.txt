# MIT License
#
# Copyright (c) 2021 Alex Vie (silvercircle@gmail.com)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.17)
option(USE_CLANG "build application with clang" ON)
project(objctest LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(JSON_BuildTests OFF CACHE INTERNAL "")

# set(CMAKE_VERBOSE_MAKEFILE ON)

#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)

find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB2 REQUIRED glib-2.0)

#find_package(Boost 1.71.0 COMPONENTS program_options)

add_compile_options(${GLIB2_CFLAGS_OTHER})

link_directories (${GLIB2_LIBRARY_DIRS})
include_directories (${GLIB2_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src)

add_executable(objctest src/pch.h src/main.cpp
        src/conf.h src/smart_pointers.cpp src/smart_pointers.h src/options.h src/options.cpp src/DataHandler.cpp src/DataHandler.h)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
target_link_libraries(objctest ${GLIB2_LIBRARIES})

