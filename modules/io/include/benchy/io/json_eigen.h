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
#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <nlohmann/json.hpp>

namespace nlohmann {

template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
struct adl_serializer<Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>>
{
    static void to_json(
        nlohmann::json& j,
        const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& matrix)
    {
        for (Eigen::Index r = 0; r < matrix.rows(); ++r) {
            if (matrix.cols() > 1) {
                nlohmann::json jrow = nlohmann::json::array();
                for (Eigen::Index c = 0; c < matrix.cols(); ++c) {
                    jrow.push_back(matrix(r, c));
                }
                j.emplace_back(std::move(jrow));
            } else {
                j.push_back(matrix(r, 0));
            }
        }
    }

    static void from_json(
        const nlohmann::json& j,
        Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& matrix)
    {
        using Scalar =
            typename Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>::Scalar;

        bool resized = false;
        auto resize = [&](size_t nrows, size_t ncols) {
            if (!resized) {
                matrix.resize(nrows, ncols);
                resized = true;
            } else {
                assert(static_cast<size_t>(matrix.rows()) == nrows);
                assert(static_cast<size_t>(matrix.cols()) == ncols);
            }
        };

        for (size_t r = 0, nrows = j.size(); r < nrows; ++r) {
            const auto& jrow = j.at(r);
            if (jrow.is_array()) {
                const size_t ncols = jrow.size();
                resize(nrows, ncols);
                for (size_t c = 0; c < ncols; ++c) {
                    const auto& value = jrow.at(c);
                    matrix(static_cast<Eigen::Index>(r), static_cast<Eigen::Index>(c)) =
                        value.get<Scalar>();
                }
            } else {
                resize(nrows, 1);
                matrix(static_cast<Eigen::Index>(r), 0) = jrow.get<Scalar>();
            }
        }
    }
};

template <typename Scalar_, int Options_, typename StorageIndex_>
struct adl_serializer<Eigen::SparseMatrix<Scalar_, Options_, StorageIndex_>>
{
    using SpMatrix = Eigen::SparseMatrix<Scalar_, Options_, StorageIndex_>;

    static void to_json(nlohmann::json& j, const SpMatrix& matrix)
    {
        auto rows = nlohmann::json::array();
        auto cols = nlohmann::json::array();
        auto vals = nlohmann::json::array();
        rows.get_ptr<json::array_t*>()->reserve(matrix.nonZeros());
        cols.get_ptr<json::array_t*>()->reserve(matrix.nonZeros());
        vals.get_ptr<json::array_t*>()->reserve(matrix.nonZeros());
        for (Eigen::Index k = 0, r = 0; k < matrix.outerSize(); ++k) {
            for (typename SpMatrix::InnerIterator it(matrix, k); it; ++it) {
                rows.push_back(it.row());
                cols.push_back(it.col());
                vals.push_back(it.value());
            }
        }
        j.emplace_back(matrix.rows());
        j.emplace_back(matrix.cols());
        j.emplace_back(std::move(rows));
        j.emplace_back(std::move(cols));
        j.emplace_back(std::move(vals));
    }

    static void from_json(const nlohmann::json& j, SpMatrix& matrix)
    {
        if (j.size() != 5) {
            throw json::other_error::create(
                502,
                "Unexpected array size: " + std::to_string(j.size()),
                &j);
        }

        const Eigen::Index num_rows = j.at(0);
        const Eigen::Index num_cols = j.at(1);
        const auto& rows = j.at(2);
        const auto& cols = j.at(3);
        const auto& vals = j.at(4);
        const size_t nnz = vals.size();
        std::vector<Eigen::Triplet<Scalar_, StorageIndex_>> triplets;
        triplets.reserve(nnz);
        for (size_t i = 0; i < nnz; ++i) {
            triplets.emplace_back(rows.at(i), cols.at(i), vals.at(i));
        }
        matrix.resize(num_rows, num_cols);
        matrix.setFromTriplets(triplets.begin(), triplets.end());
    }
};

} // namespace nlohmann