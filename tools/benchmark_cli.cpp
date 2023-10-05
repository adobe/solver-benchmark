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

// Third-party include
#include <celero/Celero.h>
#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

// System include
#include <filesystem>
#include <regex>
#include <vector>

namespace fs = std::filesystem;
namespace b = benchy::benchmark;

int add_allowed_experiments(const fs::path data_dir, const std::string regex_str)
{
    spdlog::info(
        "Generating benchmark dataset from {0} that match regex {1}",
        data_dir.string(),
        regex_str);
    std::regex regex = std::regex(regex_str);

    // Recursively searches data directory for .zst files
    std::vector<fs::path> all_zst_files;
    for (const auto& problem_dir : fs::directory_iterator(data_dir)) {
        if (problem_dir.path().filename() == "test") {
            // Skip test folder :)
            continue;
        }
        for (const auto& system_path : fs::recursive_directory_iterator(problem_dir)) {
            const auto ext = system_path.path().extension();
            if (ext == ".zst") {
                all_zst_files.push_back(system_path);
            }
        }
    }
    if (all_zst_files.empty()) {
        spdlog::critical("No .zst files found in directory {}. Exiting", data_dir.string());
        return 1;
    }

    // Filters those files using regex
    for (const auto& path : all_zst_files) {
        if (std::regex_match(path.string(), regex))
            b::BenchmarkData::instance().m_experiment_paths.push_back(path);
    }
    if (b::BenchmarkData::instance().m_experiment_paths.empty()) {
        spdlog::critical(
            "No .zst paths in {} match regex {}. Exiting",
            data_dir.string(),
            regex_str);
        return 1;
    }

    return 0;
}

int main(int argc, char** argv)
{
    struct
    {
        fs::path input_dir = fs::path(BENCHY_DATA_DIR);
        std::string regex_str = std::string("(.*.zst)");
        fs::path output_dir = fs::path(BENCHY_SOURCE_DIR) / "output";
        int log_level = 2;
    } args;

    CLI::App app{argv[0]};
    app.option_defaults()->always_capture_default();
    app.add_option("--input", args.input_dir, "Directory of dataset to run benchmark on")
        ->check(CLI::ExistingDirectory);
    app.add_option("--regex", args.regex_str, "Regex to restrict benchmark to");
    app.add_option("--output", args.output_dir, "Directory to write output csv to")
        ->check(CLI::ExistingDirectory);
    app.add_option(
        "--level",
        args.log_level,
        "Log level: 0 is most verbose, 6 is silent. Default = 2");

    CLI11_PARSE(app, argc, argv);
    spdlog::set_level(static_cast<spdlog::level::level_enum>(args.log_level));

    int status = add_allowed_experiments(args.input_dir, args.regex_str);
    if (!status) {
        b::run_benchmarks(argv[0], args.output_dir);
    }
    return 0;
}
