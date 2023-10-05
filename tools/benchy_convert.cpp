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
#include <benchy/io/load_problem.h>

// Third-party include
#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <unsupported/Eigen/SparseExtra>

// System include
#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;

int main(int argc, char const* argv[])
{
    struct
    {
        fs::path left;
        fs::path right;
        fs::path input;
        fs::path output;
    } args;

    CLI::App app{argv[0]};
    app.option_defaults()->always_capture_default();
    auto opt = app.add_option("--input", args.input, "Input linear system")
                   ->required()
                   ->check(CLI::ExistingFile);
    app.add_option(
           "--output",
           args.output,
           "Output archive of the compressed linear system. "
           "Filename should end with .zst.")
        ->required();
    CLI11_PARSE(app, argc, argv);

    auto data = [&]() -> nlohmann::json {
        if (args.input.extension() == ".zst") {
            spdlog::info("Reading linear system from compressed archive: {}", args.input.string());
            return benchy::io::load_compressed(args.input);
        } else {
            return benchy::io::load_problem(args.input);
        }
    }();

    bool update_keys = false;
    for (auto& el : data.items()) {
        // updates old keys to new version.
        if (el.key() == "lhs" || el.key() == "rhs") {
            update_keys = true;
        }
    }

    if (update_keys) {
        nlohmann::json data_updated_keys(data);
        data_updated_keys["A"] = data_updated_keys.at("lhs");
        data_updated_keys["b"] = data_updated_keys.at("rhs");
        data_updated_keys.erase("lhs");
        data_updated_keys.erase("rhs");
        data = data_updated_keys;
    }

    if (args.output.extension() == ".zst") {
        // Save as zstd-compressed binary json
        benchy::io::save_compressed(args.output, data);
    } else {
        spdlog::error("Invalid output file extension. Should be .zst");
        return 1;
    }

    return 0;
}
