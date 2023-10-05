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
// Local include
#include <benchy/benchmark/solver_structs.h>
#include <benchy/io/json_eigen.h>
#include <benchy/io/json_io.h>

// Third-party include
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <polysolve/LinearSolver.hpp>
#include <unsupported/Eigen/SparseExtra>

// System include
#include <filesystem>

namespace fs = std::filesystem;
using Scalar = double;

namespace {
template <typename CreateSolver>
void test_solver(Eigen::SparseMatrix<Scalar> A, Eigen::VectorX<Scalar> b)
{
    Eigen::VectorX<Scalar> x = Eigen::VectorX<Scalar>::Zero(b.size());
    std::unique_ptr<polysolve::LinearSolver> solver = CreateSolver::create();
    Scalar residual;
    try {
        solver->analyzePattern(A, A.rows());
        solver->factorize(A);
        solver->solve(b, x);
        residual = (A * x - b).norm();
    } catch (const std::exception& e) {
        residual = -1.0;
        spdlog::info("FAILED with exception {}", e.what());
    } catch (...) {
        residual = -1.0;
    }
    Scalar eps = 1e-10;
    REQUIRE_THAT(residual, Catch::Matchers::WithinAbs(0, eps));
}
} // namespace

TEST_CASE("test solvers", "[solver]")
{
    auto zst_path = fs::path(BENCHY_DATA_DIR) / "test/anorigami_chandelier_01_Lowpoly/0.zst";

    // Loads data
    auto data = benchy::io::load_compressed(zst_path);
    Eigen::SparseMatrix<Scalar, Eigen::ColMajor> A = data.at("A");
    Eigen::MatrixX<Scalar> b_mat = data.at("b");
    Eigen::VectorX<Scalar> b = b_mat.col(0);

    test_solver<CreateEigenSolver>(A, b);
    test_solver<CreateCholmodSolver>(A, b);
    test_solver<CreateCholmodSimplicialSolver>(A, b);
#ifdef BENCHY_WITH_ACCELERATE
    test_solver<CreateAccelerateLLTSolver>(A, b);
    test_solver<CreateAccelerateLDLTSolver>(A, b);
#endif
#ifdef POLYSOLVE_WITH_SYMPILER
    test_solver<CreateSympilerSolver>(A, b);
#endif
#ifdef BENCHY_WITH_MKL
    test_solver<CreatePardisoSolver>(A, b);
#endif
}
