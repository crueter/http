cmake_minimum_required(VERSION 3.31)

project(http)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)

add_library(http)

target_sources(http PUBLIC
    FILE_SET http TYPE CXX_MODULES FILES
    modules/util.ixx
    modules/requests.ixx
    modules/fields.ixx
    modules/enums.ixx
    modules/socket.ixx)

add_executable(test test.cpp)

target_link_libraries(test PRIVATE OpenSSL::SSL OpenSSL::Crypto http)
