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
#include <benchy/benchmark/benchmark.h>
#include <benchy/benchmark/getRSS.h>
#include <benchy/benchmark/setup.h>
#include <benchy/benchmark/solver_structs.h>
#include <benchy/io/json_eigen.h>
#include <benchy/io/json_io.h>

// Third-party include
#include <celero/Celero.h>
#include <spdlog/spdlog.h>
#include <unsupported/Eigen/SparseExtra>

// System include
#include <ctime>
#include <filesystem>

using Scalar = double;
namespace fs = std::filesystem;

namespace benchy {
namespace benchmark {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Output CSV and benchmark runner
////////////////////////////////////////////////////////////////////////////////////////////////////

void run_benchmarks(char* exe_name, fs::path output_dir)
{
    std::string filename = get_current_time() + "_results.csv";
    fs::path output_file = output_dir / filename;

    int argc = 3;
    // Command-line arguments
    std::vector<std::string> argv_s;
    std::vector<char*> argv_v;
    argv_s.push_back(exe_name);
    argv_s.push_back("-t"); // Command-line arg to write to CSV
    argv_s.push_back(output_file.string());
    for (auto& x : argv_s) {
        x.push_back('\0');
        argv_v.push_back(x.data());
    }
    spdlog::info("Running benchmarks");
    celero::Run(argc, &argv_v[0]);
    make_final_csv(output_file, output_dir);
}

std::string get_current_time()
{
    std::time_t t = std::time(nullptr);
    std::string datetime(100, 0);
    datetime.resize(
        std::strftime(&datetime[0], datetime.size(), "%Y_%m_%d_%H_%M", std::localtime(&t)));
    return datetime;
}

std::map<int, std::tuple<std::string, std::string, int>> generate_index_map()
{
    std::map<int, std::tuple<std::string, std::string, int>> index_map;
    spdlog::info("Generating index map");
    std::vector<fs::path> matrix_paths = BenchmarkData::instance().m_experiment_paths;
    for (int i = 0; i < matrix_paths.size(); ++i) {
        auto data = benchy::io::load_compressed(matrix_paths[i]);
        Eigen::SparseMatrix<Scalar, Eigen::ColMajor> A = data.at("A");
        nlohmann::json metadata = data.at("metadata");
        auto parent_path = matrix_paths[i].parent_path().filename();
        auto filename = matrix_paths[i].filename();
        auto print_path = parent_path / filename;
        index_map[i] = {print_path.string(), metadata["dataset_name"], A.nonZeros()};
    }
    return index_map;
}

void make_final_csv(fs::path celero_csv, fs::path output_dir)
{
    spdlog::info("Generating final output CSV");
    std::map<int, std::tuple<std::string, std::string, int>> index_map = generate_index_map();
    std::ifstream celero_stream(celero_csv);

    std::string filename = get_current_time() + "_benchmark_data.csv";
    fs::path output_file = output_dir / filename;
    std::ofstream output_stream(output_file);

    // Write header
    std::string line;
    std::getline(celero_stream, line);
    output_stream << line << "System Name,"
                  << "Dataset,"
                  << "Size"
                  << "\n";
    while (std::getline(celero_stream, line)) {
        // get the experiment value, 3rd cell in each line
        int experiment_value;
        std::stringstream lineStream(line);
        std::string cell;
        int cellnum = 0;
        while (std::getline(lineStream, cell, ',')) {
            if (cellnum == 2) {
                experiment_value = std::stoi(cell);
                break;
            }
            cellnum++;
        }

        // Write to new csv file
        std::tuple<std::string, std::string, int> matrix_info = index_map[experiment_value];
        output_stream << line << std::get<0>(matrix_info) << "," << std::get<1>(matrix_info) << ","
                      << std::get<2>(matrix_info) << "\n";
    }

    // Remove old csv
    std::remove(celero_csv.string().c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SolverFixture and UDM
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CreateSolver, typename SetupBenchmark>
SolverFixture<CreateSolver, SetupBenchmark>::SolverFixture()
{
    m_solver = CreateSolver::create();
    m_residual_udm.reset(new ResidualUDM());
    m_failure_udm.reset(new FailureUDM());
    m_memory_udm.reset(new MemoryUDM());
}

template <typename CreateSolver, typename SetupBenchmark>
class SolverFixture<CreateSolver, SetupBenchmark>::ResidualUDM
    : public celero::UserDefinedMeasurementTemplate<Scalar>
{
    virtual std::string getName() const override { return "Residual"; }
    virtual bool reportSize() const override { return false; };
    virtual bool reportVariance() const override { return false; };
    virtual bool reportStandardDeviation() const override { return false; };
    virtual bool reportSkewness() const override { return false; };
    virtual bool reportKurtosis() const override { return false; };
    virtual bool reportZScore() const override { return false; };
    virtual bool reportMin() const override { return false; };
    virtual bool reportMax() const override { return false; };
};

template <typename CreateSolver, typename SetupBenchmark>
class SolverFixture<CreateSolver, SetupBenchmark>::FailureUDM
    : public celero::UserDefinedMeasurementTemplate<int>
{
    virtual std::string getName() const override { return "Numerical Failure"; }
    virtual bool reportSize() const override { return false; };
    virtual bool reportVariance() const override { return false; };
    virtual bool reportStandardDeviation() const override { return false; };
    virtual bool reportSkewness() const override { return false; };
    virtual bool reportKurtosis() const override { return false; };
    virtual bool reportZScore() const override { return false; };
    virtual bool reportMin() const override { return false; };
    virtual bool reportMax() const override { return false; };
};

template <typename CreateSolver, typename SetupBenchmark>
class SolverFixture<CreateSolver, SetupBenchmark>::MemoryUDM
    : public celero::UserDefinedMeasurementTemplate<size_t>
{
    virtual std::string getName() const override { return "Physical Memory (b)"; }
    virtual bool reportSize() const override { return false; };
    virtual bool reportVariance() const override { return false; };
    virtual bool reportStandardDeviation() const override { return false; };
    virtual bool reportSkewness() const override { return false; };
    virtual bool reportKurtosis() const override { return false; };
    virtual bool reportZScore() const override { return false; };
    virtual bool reportMin() const override { return false; };
    virtual bool reportMax() const override { return false; };
};

template <typename CreateSolver, typename SetupBenchmark>
std::vector<celero::TestFixture::ExperimentValue>
SolverFixture<CreateSolver, SetupBenchmark>::getExperimentValues() const
{
    std::vector<celero::TestFixture::ExperimentValue> problemSpace;
    for (int i = 0; i < BenchmarkData::instance().m_experiment_paths.size(); i++) {
        problemSpace.push_back(i);
    }
    return problemSpace;
}

template <typename CreateSolver, typename SetupBenchmark>
void SolverFixture<CreateSolver, SetupBenchmark>::setUp(
    const celero::TestFixture::ExperimentValue& experimentValue)
{
    m_failure_count = 0;
    m_matrix_path = BenchmarkData::instance().m_experiment_paths.at(experimentValue.Value);
    auto data = benchy::io::load_compressed(m_matrix_path);
    m_A = data.at("A");
    Eigen::MatrixX<Scalar> b_mat = data.at("b");
    m_b = b_mat.col(0); // ensures only one column selected
    m_x = Eigen::VectorX<Scalar>::Zero(m_b.size());
    m_setup_status = SetupBenchmark::prepare(m_solver, m_A);
}

template <typename CreateSolver, typename SetupBenchmark>
void SolverFixture<CreateSolver, SetupBenchmark>::onExperimentEnd()
{
    // only calculate residual on solve phase
    if constexpr (std::is_same_v<SetupBenchmark, SolveOnly>) {
        Scalar r = (m_A * m_x - m_b).norm();
        m_residuals.push_back(r);
    }
}

template <typename CreateSolver, typename SetupBenchmark>
void SolverFixture<CreateSolver, SetupBenchmark>::tearDown()
{
    // Residuals averaged over iterations
    if constexpr (std::is_same_v<SetupBenchmark, SolveOnly>) {
        auto const count = static_cast<Scalar>(m_residuals.size());
        auto const avg = std::reduce(m_residuals.begin(), m_residuals.end()) / count;
        m_residual_udm->addValue(avg);
        if (avg > 1e-2) { // somewhat arbitrary definition of failure here
            this->addFailure();
        }
    } else {
        m_residual_udm->addValue(-1);
    }
    m_failure_udm->addValue(m_failure_count);
    m_memory_udm->addValue(getCurrentRSS());
    m_residuals.clear();
}

template <typename CreateSolver, typename SetupBenchmark>
std::vector<std::shared_ptr<celero::UserDefinedMeasurement>>
SolverFixture<CreateSolver, SetupBenchmark>::getUserDefinedMeasurements() const
{
    return {this->m_residual_udm, this->m_failure_udm, this->m_memory_udm};
}

template <typename CreateSolver, typename SetupBenchmark>
void SolverFixture<CreateSolver, SetupBenchmark>::addFailure()
{
    m_failure_count += 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Benchmarks
///////////////////////////////////////////////////////////////////////////////////////////////////

static const int SamplesCount = 3;
static const int IterationsCount = 3;

// Baseline, just runs for 100ms
typedef SolverFixture<CreateEigenSolver, AnalyzeOnly> BaselineFixture;
BASELINE_FIXED_F(Analyze, Base, BaselineFixture, IterationsCount, 100) {}
BASELINE_FIXED_F(Factorize, Base, BaselineFixture, IterationsCount, 100) {}
BASELINE_FIXED_F(Solve, Base, BaselineFixture, IterationsCount, 100) {}

// Cholmod Supernodal
#ifdef BENCHY_BENCHMARK_CHOLMOD
typedef SolverFixture<CreateCholmodSolver, AnalyzeOnly> CholmodAnalyzeFixture;
BENCHMARK_F(Analyze, Cholmod, CholmodAnalyzeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->analyzePattern(this->m_A, this->m_A.rows());
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Cholmod Analyze failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Cholmod Analyze failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateCholmodSolver, FactorizeOnly> CholmodFactorizeFixture;
BENCHMARK_F(Factorize, Cholmod, CholmodFactorizeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->factorize(this->m_A);
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Cholmod Factorize failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Cholmod Factorize failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateCholmodSolver, SolveOnly> CholmodSolveFixture;
BENCHMARK_F(Solve, Cholmod, CholmodSolveFixture, SamplesCount, IterationsCount)
{
    if (m_setup_status == SetupStatus::SUCCESS) {
        try {
            this->m_solver->solve(this->m_b, this->m_x);
        } catch (const std::runtime_error& e) {
            spdlog::warn(
                "Cholmod Solve failed on {} with message {}",
                m_matrix_path.string(),
                e.what());
            this->addFailure();
        } catch (...) {
            spdlog::warn("Cholmod Solve failed on {}", m_matrix_path.string());
            this->addFailure();
        }
    } else {
        this->addFailure();
    }
}

// Cholmod Simplicial
typedef SolverFixture<CreateCholmodSimplicialSolver, AnalyzeOnly> CholmodSimplicialAnalyzeFixture;
BENCHMARK_F(
    Analyze,
    CholmodSimplicial,
    CholmodSimplicialAnalyzeFixture,
    SamplesCount,
    IterationsCount)
{
    try {
        this->m_solver->analyzePattern(this->m_A, this->m_A.rows());
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Cholmod Simplicial Analyze failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Cholmod Simplicial Analyze failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateCholmodSimplicialSolver, FactorizeOnly>
    CholmodSimplicialFactorizeFixture;
BENCHMARK_F(
    Factorize,
    CholmodSimplicial,
    CholmodSimplicialFactorizeFixture,
    SamplesCount,
    IterationsCount)
{
    try {
        this->m_solver->factorize(this->m_A);
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Cholmod Simplicial Factorize failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Cholmod Simplicial Factorize failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateCholmodSimplicialSolver, SolveOnly> CholmodSimplicialSolveFixture;
BENCHMARK_F(Solve, CholmodSimplicial, CholmodSolveFixture, SamplesCount, IterationsCount)
{
    if (m_setup_status == SetupStatus::SUCCESS) {
        try {
            this->m_solver->solve(this->m_b, this->m_x);
        } catch (const std::runtime_error& e) {
            spdlog::warn(
                "Cholmod Simplicial Solve failed on {} with message {}",
                m_matrix_path.string(),
                e.what());
            this->addFailure();
        } catch (...) {
            spdlog::warn("Cholmod Simplicial Solve failed on {}", m_matrix_path.string());
            this->addFailure();
        }
    } else {
        this->addFailure();
    }
}
#endif

#ifdef BENCHY_BENCHMARK_EIGEN
// Eigen Simplicial LLT
typedef SolverFixture<CreateEigenSolver, AnalyzeOnly> EigenAnalyzeFixture;
BENCHMARK_F(Analyze, Eigen, EigenAnalyzeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->analyzePattern(this->m_A, this->m_A.rows());
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Eigen Simplicial LLT Analyze failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Eigen Simplicial LLT Analyze failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateEigenSolver, FactorizeOnly> EigenFactorizeFixture;
BENCHMARK_F(Factorize, Eigen, EigenFactorizeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->factorize(this->m_A);
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Eigen Simplicial LLT Factorize failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Eigen Simplicial LLT Factorize failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateEigenSolver, SolveOnly> EigenSolveFixture;
BENCHMARK_F(Solve, Eigen, EigenSolveFixture, SamplesCount, IterationsCount)
{
    if (m_setup_status == SetupStatus::SUCCESS) {
        try {
            this->m_solver->solve(this->m_b, m_x);
        } catch (const std::runtime_error& e) {
            spdlog::warn(
                "Eigen Simplicial LLT Solve failed on {} with message {}",
                m_matrix_path.string(),
                e.what());
            this->addFailure();
        } catch (...) {
            spdlog::warn("Eigen Simplicial LLT Solve failed on {}", m_matrix_path.string());
            this->addFailure();
        }
    } else {
        this->addFailure();
    }
}
#endif

#ifdef BENCHY_WITH_ACCELERATE
// AccelerateLLT
typedef SolverFixture<CreateAccelerateLLTSolver, AnalyzeOnly> AccelerateLLTAnalyzeFixture;
BENCHMARK_F(Analyze, AccelerateLLT, AccelerateLLTAnalyzeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->analyzePattern(this->m_A, this->m_A.rows());
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Accelerate LLT Analyze failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Accelerate LLT Analyze failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateAccelerateLLTSolver, FactorizeOnly> AccelerateLLTFactorizeFixture;
BENCHMARK_F(Factorize, AccelerateLLT, AccelerateLLTFactorizeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->factorize(this->m_A);
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Accelerate LLT Factorize failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Accelerate LLT Factorize failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateAccelerateLLTSolver, SolveOnly> AccelerateLLTSolveFixture;
BENCHMARK_F(Solve, AccelerateLLT, AccelerateLLTSolveFixture, SamplesCount, IterationsCount)
{
    if (m_setup_status == SetupStatus::SUCCESS) {
        try {
            this->m_solver->solve(this->m_b, this->m_x);

        } catch (const std::runtime_error& e) {
            spdlog::warn(
                "Accelerate LLT Solve failed on {} with message {}",
                m_matrix_path.string(),
                e.what());
            this->addFailure();
        } catch (...) {
            spdlog::warn("Accelerate LLT Solve failed on {}", m_matrix_path.string());
            this->addFailure();
        }
    } else {
        this->addFailure();
    }
}

// AccelerateLDLT
typedef SolverFixture<CreateAccelerateLDLTSolver, AnalyzeOnly> AccelerateLDLTAnalyzeFixture;
BENCHMARK_F(Analyze, AccelerateLDLT, AccelerateLDLTAnalyzeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->analyzePattern(this->m_A, this->m_A.rows());
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Accelerate LDLT Analyze failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Accelerate LDLT Analyze failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateAccelerateLDLTSolver, FactorizeOnly> AccelerateLDLTFactorizeFixture;
BENCHMARK_F(
    Factorize,
    AccelerateLDLT,
    AccelerateLDLTFactorizeFixture,
    SamplesCount,
    IterationsCount)
{
    try {
        this->m_solver->factorize(this->m_A);
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Accelerate LDLT Factorize failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Accelerate LDLT Factorize failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateAccelerateLDLTSolver, SolveOnly> AccelerateLDLTSolveFixture;
BENCHMARK_F(Solve, AccelerateLDLT, AccelerateLDLTSolveFixture, SamplesCount, IterationsCount)
{
    if (m_setup_status == SetupStatus::SUCCESS) {
        try {
            this->m_solver->solve(this->m_b, this->m_x);
        } catch (const std::runtime_error& e) {
            spdlog::warn(
                "Accelerate LDLT Solve failed on {} with message {}",
                m_matrix_path.string(),
                e.what());
            this->addFailure();
        } catch (...) {
            spdlog::warn("Accelerate LDLT Solve failed on {}", m_matrix_path.string());
            this->addFailure();
        }
    } else {
        this->addFailure();
    }
}
#endif

#ifdef BENCHY_WITH_MKL
// Pardiso
typedef SolverFixture<CreatePardisoSolver, AnalyzeOnly> PardisoAnalyzeFixture;
BENCHMARK_F(Analyze, Pardiso, PardisoAnalyzeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->analyzePattern(this->m_A, this->m_A.rows());
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "MKL Pardiso Analyze failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("MKL Pardiso Analyze failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreatePardisoSolver, FactorizeOnly> PardisoFactorizeFixture;
BENCHMARK_F(Factorize, Pardiso, PardisoFactorizeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->factorize(this->m_A);
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "MKL Pardiso Factorize failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("MKL Pardiso Factorize failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreatePardisoSolver, SolveOnly> PardisoSolveFixture;
BENCHMARK_F(Solve, Pardiso, PardisoSolveFixture, SamplesCount, IterationsCount)
{
    if (m_setup_status == SetupStatus::SUCCESS) {
        try {
            this->m_solver->solve(this->m_b, this->m_x);
        } catch (const std::runtime_error& e) {
            spdlog::warn(
                "MKL Pardiso Solve failed on {} with message {}",
                m_matrix_path.string(),
                e.what());
            this->addFailure();
        } catch (...) {
            spdlog::warn("MKL Pardiso Solve failed on {}", m_matrix_path.string());
            this->addFailure();
        }
    } else {
        this->addFailure();
    }
}
#endif

#ifdef POLYSOLVE_WITH_SYMPILER
// Sympiler
typedef SolverFixture<CreateSympilerSolver, AnalyzeOnly> SympilerAnalyzeFixture;
BENCHMARK_F(Analyze, Sympiler, SympilerAnalyzeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->analyzePattern(this->m_A, this->m_A.rows());
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Sympiler Analyze failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Sympiler Analyze failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateSympilerSolver, FactorizeOnly> SympilerFactorizeFixture;
BENCHMARK_F(Factorize, Sympiler, SympilerFactorizeFixture, SamplesCount, IterationsCount)
{
    try {
        this->m_solver->factorize(this->m_A);
    } catch (const std::runtime_error& e) {
        spdlog::warn(
            "Sympiler Factorize failed on {} with message {}",
            m_matrix_path.string(),
            e.what());
        this->addFailure();
    } catch (...) {
        spdlog::warn("Sympiler Factorize failed on {}", m_matrix_path.string());
        this->addFailure();
    }
}

typedef SolverFixture<CreateSympilerSolver, SolveOnly> SympilerSolverFixture;
BENCHMARK_F(Solve, Sympiler, SympilerSolverFixture, SamplesCount, IterationsCount)
{
    if (m_setup_status == SetupStatus::SUCCESS) {
        try {
            this->m_solver->solve(this->m_b, this->m_x);
        } catch (const std::runtime_error& e) {
            spdlog::warn(
                "Sympiler Solve failed on {} with message {}",
                m_matrix_path.string(),
                e.what());
            this->addFailure();
        } catch (...) {
            spdlog::warn("Sympiler Solve failed on {}", m_matrix_path.string());
            this->addFailure();
        }
    } else {
        this->addFailure();
    }
}
#endif

} // namespace benchmark
} // namespace benchy
