# SPDX-FileCopyrightText: Copyright 2026 crueter
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.31)

project(hydrocarbon_test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

if (PROJECT_IS_TOP_LEVEL)
    find_package(hydrocarbon REQUIRED)
endif()

# tests use jacinth for json parsing
find_package(jacinth 0.1.0 REQUIRED)

# test lib
add_library(tests STATIC)

target_sources(tests PUBLIC
    FILE_SET tests TYPE CXX_MODULES FILES tests.ixx)

target_link_libraries(tests PUBLIC hydro::carbon)

if (MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(tests PUBLIC
        /W4 /WX /we4834 /wd4702)
else()
    target_compile_options(tests PUBLIC
        -Werror=unused-result -Wextra -Werror
        -Wno-c23-extensions -Wno-unused-command-line-argument)
endif()

# need to link to stdc++exp (on glibc++ only) for std::print on mingw
if (MINGW)
    include(CheckCXXSymbolExists)

    check_cxx_symbol_exists(__GLIBCXX__ "version" HAS_LIBSTDCXX)

    if(HAS_LIBSTDCXX)
        message(STATUS "MinGW libstdc++ needs stdc++exp")
        target_link_libraries(tests PUBLIC stdc++exp)
    endif()
endif()

# test cases
set(TESTS get)

foreach(test ${TESTS})
    add_executable(${test} cases/${test}.cpp)
    target_link_libraries(${test} PRIVATE tests jacinth::jacinth)

    add_test(NAME ${test} COMMAND ${test})
endforeach()
