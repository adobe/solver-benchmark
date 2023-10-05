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
include_guard(GLOBAL)

if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    include(benchy_filter_flags)
    set(BENCHY_GLOBAL_FLAGS
        -fdiagnostics-color=always # GCC
        -fcolor-diagnostics # Clang
    )
    benchy_filter_flags(BENCHY_GLOBAL_FLAGS)
    message(STATUS "Adding global flags: ${BENCHY_GLOBAL_FLAGS}")
    add_compile_options(${BENCHY_GLOBAL_FLAGS})
endif()
