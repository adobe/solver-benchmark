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
file(GLOB_RECURSE LIB_SOURCES
    "include/*.h"
    "src/*.cpp"
)
add_library(benchy_io "${LIB_SOURCES}")
add_library(benchy::io ALIAS benchy_io)

# Include headers
target_include_directories(benchy_io PUBLIC include)

# Direct dependencies
include(eigen)
include(spdlog)
include(nlohmann_json)
include(zstd)
target_link_libraries(benchy_io PUBLIC
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
    zstd::zstd
    spdlog::spdlog
)

# Compile definitions
target_compile_definitions(benchy_io PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(benchy_io PUBLIC NOMINMAX)

# C++ standard
target_compile_features(benchy_io PUBLIC cxx_std_17)
