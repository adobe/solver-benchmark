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
#include <benchy/io/save_problem.h>
#include <benchy/io/load_problem.h>

#include <benchy/io/json_eigen.h>
#include <benchy/io/json_io.h>

// Third-party include
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <unsupported/Eigen/SparseExtra>

// System include
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

namespace {

std::vector<char> load_binary(const std::filesystem::path& filename)
{
    const auto ext = filename.extension();
    std::ifstream fl(filename, std::ios::in | std::ios::binary);
    if (!fl.is_open()) {
        throw std::runtime_error("file `" + filename.string() + "` could not be openned");
    }
    std::vector<char> buffer(std::istreambuf_iterator<char>(fl), {});
    return buffer;
}

template <typename Scalar>
void test_problem_io()
{
    int n = 100;
    int nnz = n * n / 10;
    Eigen::SparseMatrix<Scalar> A(n, n);
    Eigen::VectorX<Scalar> b(n);
    std::vector<Eigen::Triplet<Scalar>> triplets;
    triplets.reserve(nnz);

    using StorageIndex = typename Eigen::SparseMatrix<Scalar>::StorageIndex;
    std::mt19937 gen;
    std::uniform_int_distribution<StorageIndex> dist_i(0, n - 1);
    std::uniform_real_distribution<Scalar> dist_v(0, 1);
    for (int i = 0; i < nnz; ++i) {
        triplets.emplace_back(dist_i(gen), dist_i(gen), dist_v(gen));
    }
    A.setFromTriplets(triplets.begin(), triplets.end());

    for (int i = 0; i < n; ++i) {
        b(i) = dist_v(gen);
    }

    benchy::io::Problem<Scalar> problem;
    problem.A = A;
    problem.b = b;
    problem.is_symmetric_positive_definite = 0;
    problem.is_sequence_of_problems = 0;
    problem.dimension = 1;
    problem.description = "test";
    problem.dataset_name = ".";
    problem.project_url = ".";
    problem.contact_email = ".";
    benchy::io::save_problem("test.json", problem);

    nlohmann::json data = benchy::io::load_problem("test.json");
    A = data["A"];
    b = data["b"];
    REQUIRE((A.coeffs() == problem.A.coeffs()).all());
    REQUIRE(b == problem.b);

    benchy::io::save_compressed("test.zst", data);
}

} // namespace

TEST_CASE("test io", "[io]")
{
    fs::path path_A = fs::path(BENCHY_DATA_DIR) / "test/suzanne.harmonic.A.mtx";
    fs::path path_b = fs::path(BENCHY_DATA_DIR) / "test/suzanne.harmonic.b.mtx";
    fs::path path_out = fs::path(BENCHY_DATA_DIR) / "test/suzanne.harmonic.zst";
    REQUIRE(fs::exists(path_A));
    REQUIRE(fs::exists(path_b));

    nlohmann::json data;
    Eigen::SparseMatrix<double> A, b_;
    Eigen::MatrixXd b;

    REQUIRE(loadMarket(A, path_A.string()));
    REQUIRE(loadMarket(b_, path_b.string()));
    b = b_;

    data["lhs"] = A;
    data["rhs"] = b;

    // Ensures that reading Eigen matrix back from json produces the same data
    {
        Eigen::SparseMatrix<double> A2;
        Eigen::MatrixXd b2;
        Eigen::MatrixXd Ad1, Ad2;

        A2 = data["lhs"];
        b2 = data["rhs"];

        Ad1 = A;
        Ad2 = A2;
        REQUIRE(b == b2);
        REQUIRE(Ad1 == Ad2);
    }

    // Ensures that save_compressed matches existing archive
    {
        benchy::io::save_compressed("out.zst", data);
        auto buffer1 = load_binary("out.zst");
        auto buffer2 = load_binary(path_out);
        REQUIRE(buffer1 == buffer2);
    }

    // Ensures that load compressed archives produces same json data
    {
        auto data2 = benchy::io::load_compressed(path_out);
        REQUIRE(data == data2);
    }
}

TEST_CASE("problem io", "[io]")
{
    test_problem_io<double>();
    test_problem_io<float>();
}
