cmake_minimum_required(VERSION 3.10)

project(uvhttp)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

## LIBUV
include(cmake/uv.cmake)
include_directories(include)

## HTTP_PARSER
include(cmake/http-parser.cmake)

#add_subdirectory(vendor/libuv)
add_subdirectory(src)

add_subdirectory(test)
add_subdirectory(example)