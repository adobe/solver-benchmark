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
// Local include
#include <benchy/io/json_eigen.h>
#include <benchy/io/json_io.h>

// Third-party include
#include <spdlog/spdlog.h>
#include <Eigen/CholmodSupport>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// System include
#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE("benchmark solvers", "[!benchmark]")
{
    fs::path root_folder = BENCHY_DATA_DIR;

    using Scalar = double;

    for (const auto& problem_dir : fs::directory_iterator(root_folder)) {
        if (problem_dir.path().filename() == "test") {
            // Skip test folder :)
            continue;
        }
        BENCHMARK_ADVANCED(fmt::format("Problem: {}", problem_dir.path().filename().string()))
        (Catch::Benchmark::Chronometer meter)
        {
            for (const auto& system_path : fs::directory_iterator(problem_dir)) {
                const auto ext = system_path.path().extension();
                if (ext != ".zst") {
                    spdlog::warn("Unexpected file extension: '{}' (should be .zst)", ext.string());
                    continue;
                }
                Eigen::SparseMatrix<Scalar> A;
                Eigen::MatrixX<Scalar> b, x;
                {
                    auto data = benchy::io::load_compressed(system_path);
                    A = data.at("lhs");
                    b = data.at("rhs");
                }
                REQUIRE(A.cols() == b.rows());
                meter.measure([&] {
                    Eigen::CholmodSupernodalLLT<Eigen::SparseMatrix<Scalar>> solver(A);
                    x = solver.solve(b);
                    return x.sum();
                });
            }
        };
    }
}
