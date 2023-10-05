#
# Copyright 2022 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#

# Set the name of your project here
set(DOXYGEN_PROJECT_NAME Benchy)

cmake_minimum_required(VERSION 3.16)
project(${DOXYGEN_PROJECT_NAME}Docs)

find_package(Doxygen REQUIRED)

# Doxygen Awesome for fancy CSS theme
include(ExternalProject)
include(FetchContent)
ExternalProject_Add(
    doxygen-awesome
    PREFIX "${FETCHCONTENT_BASE_DIR}/doxygen-awesome"
    GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
    GIT_TAG v2.1.0
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)
ExternalProject_Get_property(doxygen-awesome SOURCE_DIR)
set(DOXYGEN_AWESOME_DIR "${SOURCE_DIR}")

# Space-separated list of public include folders used by this project. We could also get that from
# our CMake targets, but this works well enough.
set(DOXYGEN_INCLUDE_PATHS include)

# Space-separated list of input files/folders containing documentation.
set(DOXYGEN_INPUT
    ${PROJECT_SOURCE_DIR}/../modules/benchmark/include/benchy/benchmark
    ${PROJECT_SOURCE_DIR}/../modules/io/include/benchy/io
    ${PROJECT_SOURCE_DIR}/mainpage.md
)
list(JOIN DOXYGEN_INPUT " " DOXYGEN_INPUT)

# Exclude those folders from Doxygen documentation.
set(DOXYGEN_EXCLUDE
    ${PROJECT_SOURCE_DIR}/../modules/io/include/benchy/io/json_eigen.h
)
list(JOIN DOXYGEN_EXCLUDE " " DOXYGEN_EXCLUDE)

# If you want to use a different .md file as the main Doxygen page. Do not forget to include it in
# the DOXYGEN_INPUT list.
set(DOXYGEN_MDFILE_AS_MAINPAGE ${PROJECT_SOURCE_DIR}/mainpage.md)

# To customize the layout of the Doxygen website, follow instructions here:
# https://www.doxygen.nl/manual/customize.html
#
# 1. Run `doxygen -l` in your terminal;
# 2. Set DOXYGEN_LAYOUT_FILE to the generated DoxygenLayout.xml;
# 3. Add DoxygenLayout.xml to your git repo and customize its content.
set(DOXYGEN_LAYOUT_FILE "")

set(DOXYGEN_WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Output base dir for doxygen documentation")
set(DOXYGEN_OUTPUT_HTML html CACHE STRING "Output subdir for doxygen html")
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
set(DOXYFILE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/header.html)

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}) # Doxygen won't create this for us

add_custom_target(doc
                  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                  WORKING_DIRECTORY ${DOXYGEN_WORKING_DIR}
                  COMMENT "Generating docs")

add_dependencies(doc doxygen-awesome)
