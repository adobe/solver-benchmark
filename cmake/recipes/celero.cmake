#
# Copyright 2023 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#

if(TARGET celero)
    return()
endif()

message(STATUS "Third-party (external): creating target 'celero'")

option(CELERO_COMPILE_DYNAMIC_LIBRARIES "Set to ON to build Celero for dynamic linking.  Use OFF for static." OFF)

include(CPM)
CPMAddPackage(
    NAME celero
    GITHUB_REPOSITORY DigitalInBlue/Celero
    GIT_TAG v2.8.5
)

set_target_properties(celero PROPERTIES FOLDER third_party)

if(CELERO_COMPILE_DYNAMIC_LIBRARIES)
    target_compile_definitions(celero PUBLIC -DCELERO_EXPORTS)
else()
    target_compile_definitions(celero PUBLIC -DCELERO_STATIC)
endif()