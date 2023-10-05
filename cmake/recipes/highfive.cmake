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

# HighFive (https://github.com/BlueBrain/HighFive)
# License: BSL-1.0 License

if(TARGET HighFive::HighFive)
    return()
endif()

message(STATUS "Third-party (external): creating target 'HighFive::HighFive'")

option(HIGHFIVE_USE_BOOST "Enable Boost Support" OFF)
option(HIGHFIVE_UNIT_TESTS "Enable unit tests" OFF)
option(HIGHFIVE_EXAMPLES "Compile examples" OFF)
option(HIGHFIVE_BUILD_DOCS "Enable documentation building" OFF)

# Used to prevend looking for sys libraries
set(HDF5_C_LIBRARIES "")
set(HDF5_INCLUDE_DIRS "")
set(HDF5_LIBRARIES "")
set(HDF5_DEFINITIONS "")

include(CPM)
CPMAddPackage(
    NAME HighFive
    GITHUB_REPOSITORY BlueBrain/HighFive
    GIT_TAG v2.7.1
)

# Inject dependencies manually
include(eigen)
include(hdf5)
add_library(HighFive_deps INTERFACE IMPORTED GLOBAL)
target_link_libraries(HighFive_deps INTERFACE Eigen3::Eigen hdf5::hdf5)
target_link_libraries(libdeps INTERFACE HighFive_deps)
target_compile_definitions(libdeps INTERFACE H5_USE_EIGEN)

add_library(HighFive::HighFive ALIAS HighFive)
