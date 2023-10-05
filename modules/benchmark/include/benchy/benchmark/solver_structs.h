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

// Third-party include
#include <polysolve/LinearSolver.hpp>

using Scalar = double;

///
/// Thin wrapper over Eigen::SimplicialLDLT
///
struct CreateEigenSolver
{
    static std::unique_ptr<polysolve::LinearSolver> create()
    {
        return polysolve::LinearSolver::create("Eigen::SimplicialLDLT", "");
    }
};

///
/// Thin wrapper over Eigen::CholmodSupernodalLLT
///
struct CreateCholmodSolver
{
    static std::unique_ptr<polysolve::LinearSolver> create()
    {
        return polysolve::LinearSolver::create("Eigen::CholmodSupernodalLLT", "");
    }
};

///
/// Thin wrapper over Eigen::CholmodSimplicialLLT
///
struct CreateCholmodSimplicialSolver
{
    static std::unique_ptr<polysolve::LinearSolver> create()
    {
        return polysolve::LinearSolver::create("Eigen::CholmodSimplicialLLT", "");
    }
};

#ifdef BENCHY_WITH_ACCELERATE
///
/// Thin wrapper over Eigen::AccelerateLLT
///
struct CreateAccelerateLLTSolver
{
    static std::unique_ptr<polysolve::LinearSolver> create()
    {
        return polysolve::LinearSolver::create("Eigen::AccelerateLLT", "");
    }
};

///
/// Thin wrapper over Eigen::AccelerateLDLT
///
struct CreateAccelerateLDLTSolver
{
    static std::unique_ptr<polysolve::LinearSolver> create()
    {
        return polysolve::LinearSolver::create("Eigen::AccelerateLDLT", "");
    }
};
#endif

#ifdef POLYSOLVE_WITH_SYMPILER
///
/// Thin wrapper over Sympiler
///
struct CreateSympilerSolver
{
    static std::unique_ptr<polysolve::LinearSolver> create()
    {
        return polysolve::LinearSolver::create("Sympiler", "");
    }
};
#endif

#ifdef BENCHY_WITH_MKL
///
/// Thin wrapper over Eigen::PardisoLLT
///
struct CreatePardisoSolver
{
    static std::unique_ptr<polysolve::LinearSolver> create()
    {
        return polysolve::LinearSolver::create("Eigen::PardisoLLT", "");
    }
};
#endif
