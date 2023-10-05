#
# Copyright 2023 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#

# hdf5
# License: BSD-3

if(TARGET hdf5::hdf5)
    return()
endif()

message(STATUS "Third-party: creating target 'hdf5'")

# option(HDF5_NO_PACKAGES "CPACK - Disable packaging" OFF)
option(HDF5_ENABLE_DEPRECATED_SYMBOLS "Enable deprecated public API symbols" OFF)
# option(HDF5_PACKAGE_EXTLIBS "CPACK - include external libraries" OFF)
option(BUILD_TESTING "Build HDF5 Unit Testing" OFF)
option(HDF5_TEST_SERIAL "Execute non-parallel tests" OFF)
option(HDF5_TEST_TOOLS "Execute tools tests" OFF)
option(HDF5_TEST_EXAMPLES "Execute tests on examples" OFF)
option(HDF5_TEST_SWMR "Execute SWMR tests" OFF)
option(HDF5_TEST_PARALLEL "Execute parallel tests" OFF)
option(HDF5_TEST_FORTRAN "Execute fortran tests" OFF)
option(HDF5_TEST_CPP "Execute cpp tests" OFF)
option(HDF5_TEST_JAVA "Execute java tests" OFF)
option(HDF5_BUILD_TOOLS  "Build HDF5 Tools" OFF)
option(HDF5_BUILD_EXAMPLES  "Build HDF5 Library Examples" OFF)
option(HDF5_BUILD_HL_LIB  "Build HIGH Level HDF5 Library" OFF)
option(HDF5_USE_PREGEN "Use pre-generated Files" OFF)
option(HDF5_GENERATE_HEADERS "Rebuild Generated Files" OFF)

option(HDF5_ENABLE_Z_LIB_SUPPORT "Enable Zlib Filters" ON)
# option(HDF5_ENABLE_SZIP_SUPPORT "Use SZip Filter" ON)
# option(HDF5_ENABLE_SZIP_ENCODING "Use SZip Encoding" ON)

# Tell hdf5 that we are manually overriding certain settings
set (HDF5_EXTERNALLY_CONFIGURED 1)

# Setup all necessary overrides for zlib so that HDF5 uses our
# internally compiled zlib rather than any other version
if(HDF5_ENABLE_Z_LIB_SUPPORT)
    include(miniz)
    set(HDF5_LIB_DEPENDENCIES "")
    set(H5_ZLIB_HEADER "miniz.h")
    set(ZLIB_INCLUDE_DIRS "")
    set(ZLIB_LIBRARIES miniz::miniz)
endif()

include(CPM)
CPMAddPackage(
    NAME hdf5
    GITHUB_REPOSITORY HDFGroup/hdf5
    GIT_TAG hdf5-1_14_1-2
)

function(hdf5_import_target)
    macro(push_variable var value)
        if(DEFINED CACHE{${var}})
            set(HDF5_OLD_${var}_VALUE "${${var}}")
            set(HDF5_OLD_${var}_TYPE CACHE_TYPE)
        elseif(DEFINED ${var})
            set(HDF5_OLD_${var}_VALUE "${${var}}")
            set(HDF5_OLD_${var}_TYPE NORMAL_TYPE)
        else()
            set(HDF5_OLD_${var}_TYPE NONE_TYPE)
        endif()
        set(${var} "${value}" CACHE PATH "" FORCE)
    endmacro()

    macro(pop_variable var)
        if(HDF5_OLD_${var}_TYPE STREQUAL CACHE_TYPE)
            set(${var} "${HDF5_OLD_${var}_VALUE}" CACHE PATH "" FORCE)
        elseif(HDF5_OLD_${var}_TYPE STREQUAL NORMAL_TYPE)
            unset(${var} CACHE)
            set(${var} "${HDF5_OLD_${var}_VALUE}")
        elseif(HDF5_OLD_${var}_TYPE STREQUAL NONE_TYPE)
            unset(${var} CACHE)
        else()
            message(FATAL_ERROR "Trying to pop a variable that has not been pushed: ${var}")
        endif()
    endmacro()

    push_variable(BUILD_TESTING OFF)
    FetchContent_MakeAvailable(hdf5)
    pop_variable(BUILD_TESTING)
endfunction()

hdf5_import_target()

target_link_libraries(hdf5-static PUBLIC miniz::miniz)
add_library(hdf5::hdf5 ALIAS hdf5-static)
