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
include(catch2)
include(celero)
include(polysolve)
benchy_include_modules(io benchmark)

FetchContent_GetProperties(catch2)
include("${catch2_SOURCE_DIR}/extras/Catch.cmake")

file(GLOB TEST_SOURCES "*.h" "*.cpp")
add_executable(benchy_tests ${TEST_SOURCES})

target_link_libraries(benchy_tests PUBLIC
    Catch2::Catch2WithMain
    benchy::io
    benchy::benchmark
    polysolve::polysolve
    SuiteSparse::CHOLMOD
)

add_dependencies(benchy_tests)

target_compile_definitions(benchy_tests
    PUBLIC
        CATCH_CONFIG_ENABLE_BENCHMARKING
    PRIVATE
        BENCHY_DATA_DIR="${BENCHY_DATA_FOLDER}"
        BENCHY_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

catch_discover_tests(benchy_tests)
