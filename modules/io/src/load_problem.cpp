/*
 * Copyright 2023 Adobe. All rights reserved.
 * This file is licensed to you under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 * OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */
#include <benchy/io/load_problem.h>

#include <benchy/io/json_eigen.h>

#include <Eigen/Sparse>

#include <fstream>

namespace benchy {
namespace io {

namespace {

template <typename Scalar>
Eigen::SparseMatrix<Scalar> load_matrix(const nlohmann::json& data)
{
    std::vector<Eigen::Triplet<Scalar>> triplets;
    triplets.reserve(data["triplets"].size());
    for (int i = 0; i < data["triplets"].size(); ++i) {
        const auto& entry = data["triplets"][i];
        const int row = entry[0];
        const int col = entry[1];
        if (entry.size() != 3) {
            throw std::runtime_error("Invalid triplet size");
        }
        if constexpr (std::is_same_v<Scalar, float>) {
            triplets.emplace_back(row, col, std::stof(entry[2].get<std::string>()));
        } else {
            triplets.emplace_back(row, col, std::stod(entry[2].get<std::string>()));
        }
    }
    Eigen::SparseMatrix<Scalar> A(data["rows"], data["cols"]);
    A.setFromTriplets(triplets.begin(), triplets.end());
    return A;
}

template <typename Scalar>
Eigen::VectorX<Scalar> load_vector(const nlohmann::json& data)
{
    Eigen::VectorX<Scalar> res(data.size());
    for (int i = 0; i < data.size(); ++i) {
        if constexpr (std::is_same_v<Scalar, float>) {
            res[i] = std::stof(data[i].get<std::string>());
        } else {
            res[i] = std::stod(data[i].get<std::string>());
        }
    }
    return res;
}

} // namespace

// Decompress a zstd archive and load uncompressed messagepack as json
nlohmann::json load_problem(const std::filesystem::path& filename)
{
    nlohmann::json data = nlohmann::json::parse(std::ifstream(filename));
    nlohmann::json& metadata = data["metadata"];
    if (!metadata.contains("raw_dump_version")) {
        throw std::runtime_error(
            "Attempting to read a problem that was not saved with `save_problem()`");
    }
    int version_number = metadata["raw_dump_version"];
    metadata["version_number"] = version_number;
    metadata.erase("raw_dump_version");

    if (metadata["scalar_type"] == "double") {
        data["A"] = load_matrix<double>(data["A"]);
        data["b"] = load_vector<double>(data["b"]);
    } else {
        data["A"] = load_matrix<float>(data["A"]);
        data["b"] = load_vector<float>(data["b"]);
    }

    return data;
}

} // namespace io
} // namespace benchy
