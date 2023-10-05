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
file(GLOB_RECURSE LIB_SOURCES
    "include/*.h"
    "src/*.cpp"
)
add_library(benchy_benchmark "${LIB_SOURCES}")
add_library(benchy::benchmark ALIAS benchy_benchmark)

# Include headers
target_include_directories(benchy_benchmark PUBLIC include)

# Direct dependencies
include(spdlog)
include(polysolve)
include(celero)
benchy_include_modules(io)
target_link_libraries(benchy_benchmark
    PUBLIC
        spdlog::spdlog
        benchy::io
        celero
        polysolve::polysolve
)

# Compile definitions
target_compile_definitions(benchy_benchmark
    PUBLIC
        _USE_MATH_DEFINES
        NOMINMAX
    PRIVATE
        BENCHY_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
        BENCHY_DATA_DIR="${BENCHY_DATA_FOLDER}"
)
if(BENCHY_BENCHMARK_CHOLMOD)
    target_compile_definitions(benchy_benchmark PUBLIC BENCHY_BENCHMARK_CHOLMOD)
endif()
if(BENCHY_BENCHMARK_EIGEN)
    target_compile_definitions(benchy_benchmark PUBLIC BENCHY_BENCHMARK_EIGEN)
endif()
if(BENCHY_WITH_ACCELERATE)
    target_compile_definitions(benchy_benchmark PUBLIC BENCHY_WITH_ACCELERATE)
endif()
if(BENCHY_WITH_MKL)
    target_compile_definitions(benchy_benchmark PUBLIC BENCHY_WITH_MKL)
endif()

# C++ standard
target_compile_features(benchy_benchmark PUBLIC cxx_std_17)
