/*
 * Copyright 2022 Adobe. All rights reserved.
 * This file is licensed to you under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 * OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */
#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>

namespace benchy {
namespace io {

// Serialize json to messagepack and compressed with zstd
void save_compressed(const std::filesystem::path& filename, const nlohmann::json& json);

// Decompress a zstd archive and load uncompressed messagepack as json
nlohmann::json load_compressed(const std::filesystem::path& filename);

} // namespace io
} // namespace benchy