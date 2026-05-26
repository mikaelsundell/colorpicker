# Copyright 2022-present Contributors to the colorpicker project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/colorpicker

cmake_minimum_required(VERSION 3.27)

set(tool_name "genicc")

find_package(Lcms2 REQUIRED)

add_executable(${tool_name}
    main.cpp
)

set_target_properties(${tool_name} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(${tool_name}
    PRIVATE
        -Wno-deprecated-register
)

target_include_directories(${tool_name}
    PRIVATE
        ${LCMS2_INCLUDE_DIR}
)

target_link_libraries(${tool_name}
    PRIVATE
        ${LCMS2_LIBRARY}
)