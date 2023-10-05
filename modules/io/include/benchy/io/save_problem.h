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

#include <Eigen/Sparse>

#include <fstream>
#include <iostream>
#include <string>

namespace benchy {
namespace io {

///
/// Lightweight class representing a linear system and some metadata.
///
/// @tparam     Scalar    Problem scalar type (float or double).
///
template <typename Scalar>
struct Problem
{
    /// Left-hand side sparse matrix.
    Eigen::SparseMatrix<Scalar> A;

    /// Right-hand side dense matrix. To save multiple rhs, please save separate problems.
    Eigen::VectorX<Scalar> b;

    /// Whether the sparse matrix A is supposed to be SPD.
    int is_symmetric_positive_definite = -1;

    /// Whether the problem comes from a sequence (e.g. Newton solves).
    int is_sequence_of_problems = -1;

    /// Dimensionality of the underlying problem (typically 2D or 3D).
    int dimension = 0;

    /// Human-readable description of the problem.
    std::string description;

    /// Short name of the dataset used to generate this problem.
    std::string dataset_name;

    /// URL of the project/source-code used to generate this problem.
    std::string project_url;

    /// Contact email of the person who generated this system.
    std::string contact_email;
};

///
/// Saves a linear system and associated metadata.
///
/// To save a specific linear system, you can use the following example code:
/// @code
/// benchy::io::Problem<double> problem;
/// problem.A = A;
/// problem.b = b;
/// problem.is_symmetric_positive_definite = 1;
/// problem.is_sequence_of_problems = 0;
/// problem.dimension = 3;
/// problem.description = "Linear elasticity simulation in 3D";
/// problem.dataset_name = "squishy_cube";
/// problem.project_url = "https://github.com/polyfem/polyfem/";
/// problem.contact_email = "my.name@gmail.com";
/// benchy::io::save_problem("my_problem.json", problem);
/// @endcode
///
/// @param[in]  filename  Filename to save the problem to.
/// @param[in]  problem   Container describing the linear system to save.
///
/// @tparam     Scalar    Problem scalar type (float or double).
///
/// @return     True if the problem was successfully saved, False otherwise.
///
template <typename Scalar>
bool save_problem(const std::string& filename, const Problem<Scalar>& problem)
{
    // Check that all metadata is set
    if (problem.A.size() == 0) {
        std::cerr << "Matrix A is empty" << std::endl;
        return false;
    }
    if (problem.b.size() == 0) {
        std::cerr << "Matrix b is empty" << std::endl;
        return false;
    }
    if (problem.is_symmetric_positive_definite != 0 &&
        problem.is_symmetric_positive_definite != 1) {
        std::cerr << "problem.is_symmetric_positive_definite must be 0 or 1" << std::endl;
        return false;
    }
    if (problem.is_sequence_of_problems != 0 && problem.is_sequence_of_problems != 1) {
        std::cerr << "problem.is_sequence_of_problems must be 0 or 1" << std::endl;
        return false;
    }
    if (problem.dimension <= 0) {
        std::cerr << "problem.dimension must be positive" << std::endl;
        return false;
    }
    if (problem.description.empty()) {
        std::cerr << "problem.description is empty" << std::endl;
        return false;
    }
    if (problem.dataset_name.empty()) {
        std::cerr << "problem.dataset_name is empty" << std::endl;
        return false;
    }
    if (problem.project_url.empty()) {
        std::cerr << "problem.project_url is empty" << std::endl;
        return false;
    }
    if (problem.contact_email.empty()) {
        std::cerr << "problem.contact_email is empty" << std::endl;
        return false;
    }
    static_assert(
        std::is_same<Scalar, float>::value || std::is_same<Scalar, double>::value,
        "Scalar must be float or double");

    // Write problem to file
    std::ofstream out(filename, std::ios::out);
    if (!out) {
        std::cerr << "Could not open file " << filename << std::endl;
        return false;
    }

    std::vector<Eigen::Triplet<Scalar>> triplets;
    triplets.reserve(problem.A.nonZeros());
    for (int j = 0; j < problem.A.outerSize(); ++j) {
        for (typename Eigen::SparseMatrix<Scalar>::InnerIterator it(problem.A, j); it; ++it) {
            triplets.emplace_back(it.row(), it.col(), it.value());
        }
    }

    out << std::hexfloat << std::boolalpha;
    out << "{\"metadata\": {"
        << "\"is_symmetric_positive_definite\": " << problem.is_symmetric_positive_definite << ", "
        << "\"is_sequence_of_problems\": " << problem.is_sequence_of_problems << ", "
        << "\"dimension\": " << problem.dimension << ", "
        << "\"scalar_type\": \"" << (std::is_same<Scalar, float>::value ? "float" : "double")
        << "\", "
        << "\"description\": \"" << problem.description << "\", "
        << "\"dataset_name\": \"" << problem.dataset_name << "\", "
        << "\"project_url\": \"" << problem.project_url << "\", "
        << "\"contact_email\": \"" << problem.contact_email << "\", "
        << "\"raw_dump_version\": " << 2 << "}, \"A\":{ \"rows\":" << problem.A.rows()
        << ", \"cols\":" << problem.A.cols() << ", \"nnz\":" << problem.A.nonZeros()
        << ", \"triplets\":[";
    for (int i = 0; i < triplets.size(); ++i) {
        out << "[" << triplets[i].row() << ", " << triplets[i].col() << ", \""
            << triplets[i].value() << "\"]";
        if (i < triplets.size() - 1) {
            out << ", ";
        }
    }
    out << "]}, \"b\":[";
    for (int i = 0; i < problem.b.size(); ++i) {
        out << "\"" << problem.b(i) << "\"";
        if (i < problem.b.size() - 1) {
            out << ", ";
        }
    }
    out << "]}";

    return true;
}

} // namespace io
} // namespace benchy
