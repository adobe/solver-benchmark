#
# Copyright 2022 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#
# Detects whether this is a top-level project
get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    set(BENCHY_TOPLEVEL_PROJECT OFF)
else()
    set(BENCHY_TOPLEVEL_PROJECT ON)
endif()

# Check required CMake version
cmake_minimum_required(VERSION 3.24)

# Enable ccache if available
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/benchy/benchy_setup_ccache.cmake)

option(BENCHY_APPLE_MKL "Used to build benchmarks on apple silicon targeting x86"             OFF)
# Include user-provided default options if available. We do that before the main
# `project()` so that we can define the C/C++ compilers from the option file.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/SolverBenchmarkOptions.cmake)
    if(BENCHY_TOPLEVEL_PROJECT)
        message(STATUS "Using local options file: ${CMAKE_CURRENT_SOURCE_DIR}/SolverBenchmarkOptions.cmake")
        include(${CMAKE_CURRENT_SOURCE_DIR}/SolverBenchmarkOptions.cmake)
    else()
        message(WARNING "Ignoring local options file for non-top-level build: "
                        "${CMAKE_CURRENT_SOURCE_DIR}/SolverBenchmarkOptions.cmake")
    endif()
endif()

# Main project name
project(Benchy)

if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" AND APPLE)
    set(BENCHY_ON_APPLE_SILICON ON)
    set(BENCHY_NOT_ON_APPLE_SILICON OFF)
else()
    set(BENCHY_ON_APPLE_SILICON OFF)
    set(BENCHY_NOT_ON_APPLE_SILICON ON)
endif()

# Build options
option(BENCHY_TOOLS             "Build cli tools"                                                ON)
option(BENCHY_TESTS             "Build unit tests"                                               ON)
option(BENCHY_DOCS              "Build doxygen documentation"                                   OFF)
option(BENCHY_WITH_HDF5         "Enable HDF5 dependency"                                        OFF)
option(BENCHY_WITH_ACCELERATE   "Enable Accelerate on Apple Silicon"     ${BENCHY_ON_APPLE_SILICON})
option(BENCHY_WITH_MKL          "Enable building with MKL"           ${BENCHY_NOT_ON_APPLE_SILICON})
option(BENCHY_BENCHMARK_CHOLMOD "Benchmark cholmod library"                                      ON)
option(BENCHY_BENCHMARK_EIGEN   "Benchmark with eigen (very slow)"                               ON)

if(BENCHY_APPLE_MKL)
    set(BENCHY_WITH_ACCELERATE OFF)
    set(CMAKE_HOST_SYSTEM_PROCESSOR "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR "x86_64")
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    set(BENCHY_WITH_MKL ON)
endif()

list(PREPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/recipes)
list(PREPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/benchy)

# Register project source/binary dir in global properties, so they can be
# retrieved from Lagrange CMake functions when called by client code.
set_property(GLOBAL PROPERTY __benchy_source_dir ${PROJECT_SOURCE_DIR})
set_property(GLOBAL PROPERTY __benchy_binary_dir ${PROJECT_BINARY_DIR})
set_property(GLOBAL PROPERTY __benchy_module_path ${CMAKE_MODULE_PATH})

include(benchy_cpm_cache)
include(polysolve)
include(benchy_include_modules)
include(benchy_download_data)
include(benchy_global_flags)
# Tools
if(BENCHY_TOOLS)
    add_subdirectory(tools)
endif()

# Tests
if(BENCHY_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

if(BENCHY_DOCS)
    add_subdirectory(docs)
endif()
