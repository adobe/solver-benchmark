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
include(cli11)
include(spdlog)
include(celero)
benchy_include_modules(io benchmark)

add_executable(benchy_convert benchy_convert.cpp)
target_link_libraries(benchy_convert PUBLIC
    CLI11::CLI11
    spdlog::spdlog
    benchy::io
)

add_executable(benchmark_cli benchmark_cli.cpp)
target_link_libraries(benchmark_cli PUBLIC
    CLI11::CLI11
    spdlog::spdlog
    benchy::io
    benchy::benchmark
    celero
)

add_dependencies(benchmark_cli)
target_compile_definitions(benchmark_cli 
    PRIVATE 
        BENCHY_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
        BENCHY_DATA_DIR="${BENCHY_DATA_FOLDER}"
)

if(BENCHY_WITH_HDF5)
    include(highfive)
    target_link_libraries(benchy_convert PUBLIC HighFive::HighFive)
    target_compile_definitions(benchy_convert PUBLIC BENCHY_WITH_HDF5)
endif()
