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
#include <benchy/benchmark/setup.h>
#include <celero/Celero.h>
#include <polysolve/LinearSolver.hpp>
#include <unsupported/Eigen/SparseExtra>

// System include
#include <filesystem>
#include <vector>

using Scalar = double;

namespace fs = std::filesystem;
namespace benchy {
namespace benchmark {

///
/// Collection of benchmarks for linear solvers using Celero
///
/// @tparam CreateSolver type of solver to use in benchmark
/// @tparam SetupBenchmark type of computation phase: Analyze, Factorize, Solve
///
template <typename CreateSolver, typename SetupBenchmark>
class SolverFixture : public celero::TestFixture
{
public:
    ///
    /// User-defined measurement of absolute residual error: `(b - Ax).norm()`
    ///
    class ResidualUDM;

    ///
    /// User-defined measurement of whether solver failed or not
    ///
    class FailureUDM;

    ///
    /// User-defined measurement of virtual memory using getRSS
    ///
    class MemoryUDM;

    ///
    /// Default constructor
    ///
    SolverFixture();

    ///
    /// Stores internal mapping to list of linear system file paths
    ///
    /// Celero allows for running each benchmark multiple times across varying
    /// ExperimentValues. The BenchmarkData class stores a list of system paths, and during
    /// benchmarking the ExperimentValue indexes into that list to loads the matrix.
    ///
    std::vector<celero::TestFixture::ExperimentValue> getExperimentValues() const override;

    ///
    /// Loads .zst file and populates m_A, m_b fields
    ///
    virtual void setUp(const celero::TestFixture::ExperimentValue& experimentValue) override;

    ///
    /// Calculates residual if solve phase is being benchmarked
    ///
    virtual void onExperimentEnd() override;

    ///
    /// Compiles user-defined measurements after benchmark completes
    ///
    virtual void tearDown() override;

    ///
    /// Returns all user-defined measurements to celero after benchmarking
    ///
    virtual std::vector<std::shared_ptr<celero::UserDefinedMeasurement>>
    getUserDefinedMeasurements() const override;

    ///
    /// Increments counter when a benchmark fails
    ///
    void addFailure();

    /// Matrix of system being benchmarked
    Eigen::SparseMatrix<Scalar, Eigen::ColMajor> m_A;

    /// Right-hand side vector of system being benchmarked
    Eigen::VectorX<Scalar> m_b;

    /// Solution vector of system being benchmarked
    Eigen::VectorX<Scalar> m_x;

    /// Path to .zst file of system being benchmarked
    fs::path m_matrix_path;

    /// Solver used in current benchmark
    std::unique_ptr<polysolve::LinearSolver> m_solver;

    /// Vector containing residuals of all iterations. Only nonempty during "solve" benchmarks
    std::vector<Scalar> m_residuals;

    /// Number of failures across all benchmark iterations
    int m_failure_count;

    /// Whether solver failed during setup or not
    SetupStatus m_setup_status;

    /// User-defined measuremnent of residual
    std::shared_ptr<ResidualUDM> m_residual_udm;

    /// User-defined measurement of benchmark failure
    std::shared_ptr<FailureUDM> m_failure_udm;

    /// User-defined measurement of physical memory usage
    std::shared_ptr<MemoryUDM> m_memory_udm;
};

///
/// Singleton class to store list of linear system filenames
///
class BenchmarkData
{
public:
    /// @brief
    /// @return Instance of benchmark data
    static BenchmarkData& instance()
    {
        static BenchmarkData s_x;
        return s_x;
    }

public:
    /// Holds vector of paths to all systems that will be benchmarked
    std::vector<std::filesystem::path> m_experiment_paths;

private:
    BenchmarkData() = default;
    ~BenchmarkData() = default;
};

///
/// Runs benchmarks using Celero
/// @param[in] exe_name Filename of benchmark_cli executable
/// @param[in] output_dir Directory to write output csv to
///
void run_benchmarks(char* exe_name, fs::path output_dir);

///
/// Helper method to add current time to output filenames
///
std::string get_current_time();

///
/// Generates mapping from ExperimentValues to linear system info
/// @param[in] output_dir Directory to write CSV to
/// @returns Map from experiment values to (System name, dataset name, # of nonzeros) tuple
///
/// Each benchmark is ran across a range of what Celero calls ExperimentValues.
/// These ExperimentValues are basically just ints. Benchy uses these experiment
/// values to associate each benchmark with a specific linear system.
///
/// However, simply including the ExperimentValue in the output CSV would be
/// difficult to interpret, but Celero only natively allows for writing the
/// numeric ExperimentValue to the output CSV. In order to associate each
/// benchmark with the name of the system it ran on in the output CSV, this
/// function creates a map from those ExperimentValues to the systems themselves.
///
/// Kind of a hack. May be unnecessary depending on how this issue gets resolved:
/// https://github.com/DigitalInBlue/Celero/issues/169
/// https://github.com/DigitalInBlue/Celero/issues/21 also describes core issue
///
std::map<int, std::tuple<std::string, std::string, int>> generate_index_map();

/// Combines index map and celero csv into final output csv
/// @param[in] celero_csv CSV containing information from benchmark provided by Celero
/// @param[in] output_dir Directory to output final CSV to
void make_final_csv(fs::path celero_csv, fs::path output_dir);

} // namespace benchmark
} // namespace benchy