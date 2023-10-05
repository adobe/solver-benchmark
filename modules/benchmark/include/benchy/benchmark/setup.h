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

#pragma once

// Third-party include
#include <polysolve/LinearSolver.hpp>
#include <unsupported/Eigen/SparseExtra>

using Scalar = double;

namespace benchy {
namespace benchmark {
enum class SetupStatus { SUCCESS, FAILURE };

///
/// Setup for analysis benchmarks
///
struct AnalyzeOnly
{
    static SetupStatus prepare(
        std::unique_ptr<polysolve::LinearSolver>& solver,
        Eigen::SparseMatrix<Scalar, Eigen::ColMajor>& A)
    {
        return SetupStatus::SUCCESS;
    }
};

///
/// Setup for factorize benchmarks -- calls `analyzePattern()`
///
struct FactorizeOnly
{
    static SetupStatus prepare(
        std::unique_ptr<polysolve::LinearSolver>& solver,
        Eigen::SparseMatrix<Scalar, Eigen::ColMajor>& A)
    {
        solver->analyzePattern(A, A.rows());
        return SetupStatus::SUCCESS;
    }
};

///
/// Setup for solve benchmarks -- calls `analyzePattern()` and `factorize()`
///
struct SolveOnly
{
    static SetupStatus prepare(
        std::unique_ptr<polysolve::LinearSolver>& solver,
        Eigen::SparseMatrix<Scalar, Eigen::ColMajor>& A)
    {
        SetupStatus status = SetupStatus::SUCCESS;
        solver->analyzePattern(A, A.rows());
        try {
            solver->factorize(A);
        } catch (const std::runtime_error& e) {
            status = SetupStatus::FAILURE;
        }
        return status;
    }
};
} // namespace benchmark
} // namespace benchy