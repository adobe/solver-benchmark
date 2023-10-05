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
#include <benchy/io/json_io.h>

#include <spdlog/spdlog.h>

#include <exception>
#include <fstream>

extern "C" {
#include <zstd.h>
}

namespace benchy {
namespace io {

namespace {

std::vector<uint8_t> compress(const std::vector<uint8_t>& src)
{
    spdlog::info("Compressing binary data");
    const auto level = ZSTD_defaultCLevel();
    std::vector<uint8_t> dst(ZSTD_compressBound(src.size()));
    const auto code = ZSTD_compress(dst.data(), dst.size(), src.data(), src.size(), level);
    if (ZSTD_isError(code)) {
        throw std::runtime_error(
            fmt::format("[compress] Compression error: {}", ZSTD_getErrorName(code)));
    }
    dst.resize(code);
    return dst;
}

std::vector<char> decompress(const std::vector<char>& src)
{
    const auto dst_size = ZSTD_getFrameContentSize(src.data(), src.size());
    if (dst_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error(fmt::format("[decompress] Content size unkown"));
    }
    if (dst_size == ZSTD_CONTENTSIZE_ERROR) {
        throw std::runtime_error(
            fmt::format("[decompress] Error occurred when trying to determine content size"));
    }
    std::vector<char> dst(dst_size);
    const auto code = ZSTD_decompress(dst.data(), dst.size(), src.data(), src.size());
    if (ZSTD_isError(code)) {
        throw std::runtime_error(
            fmt::format("[decompress] Decompression error: {}", ZSTD_getErrorName(code)));
    }
    if (code != dst.size()) {
        throw std::runtime_error(
            fmt::format("[decompress] Mismatched decompressed size: {} / {}", code, dst.size()));
    }
    return dst;
}

} // namespace

void save_compressed(const std::filesystem::path& filename, const nlohmann::json& json)
{
    const auto ext = filename.extension();
    if (ext != ".zst") {
        spdlog::warn("Unexpected file extension: '{}' (should be .zst)", ext.string());
    }
    std::ofstream fl(filename, std::ios::out | std::ios::binary);
    if (!fl.is_open()) {
        throw std::runtime_error("file `" + filename.string() + "` could not be opened");
    }
    spdlog::info("Converting to msgpack");
    std::vector<uint8_t> msgpack = compress(nlohmann::json::to_msgpack(json));
    spdlog::info("Saving to disk: {}", filename.filename().string());
    fl.write(reinterpret_cast<char*>(msgpack.data()), msgpack.size());
    spdlog::info("Done!");
}

nlohmann::json load_compressed(const std::filesystem::path& filename)
{
    const auto ext = filename.extension();
    if (ext != ".zst") {
        spdlog::warn("Unexpected file extension: '{}' (should be .zst)", ext.string());
    }
    std::ifstream fl(filename, std::ios::in | std::ios::binary);
    if (!fl.is_open()) {
        throw std::runtime_error("file `" + filename.string() + "` could not be opened");
    }
    static_assert(sizeof(char) == sizeof(uint8_t));
    std::vector<char> msgpack(std::istreambuf_iterator<char>(fl), {});
    return nlohmann::json::from_msgpack(decompress(msgpack));
}

} // namespace io
} // namespace benchy
