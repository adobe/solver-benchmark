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
function(benchy_include_modules)
    get_property(benchy_module_path GLOBAL PROPERTY __benchy_module_path)
    set(CMAKE_MODULE_PATH ${benchy_module_path})
    foreach(name IN ITEMS ${ARGN})
        if(NOT TARGET benchy::${name})
            get_property(benchy_source_dir GLOBAL PROPERTY __benchy_source_dir)
            get_property(benchy_binary_dir GLOBAL PROPERTY __benchy_binary_dir)
            add_subdirectory(${benchy_source_dir}/modules/${name} ${benchy_binary_dir}/modules/benchy_${name})
        endif()

        if(NOT TARGET benchy::${name})
            message(FATAL_ERROR "Failed to create benchy module: ${name}")
        endif()
    endforeach()
endfunction()
