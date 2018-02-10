cmake_minimum_required(VERSION 2.8)

project(uvhttp)


option(UVHTTP_SYSTEM_UV "Use system libuv" on)
option(UVHTTP_BUILD_TEST "Build tests" off)
option(UVHTTP_BUILD_EXAMPLE "Build example" off)
option(UVHTTP_BUILD_DEBUG "build with debugging ouput" off)
option(UVHTTP_TLS "build https" off)

## Libuv
if (NOT ${UVHTTP_SYSTEM_UV})
	message(STATUS "Using local libuv")
	include(cmake/uv.cmake)
else()
	message(STATUS "Using system libuv")
endif (NOT ${UVHTTP_SYSTEM_UV})


include_directories(include)

## HTTP_PARSER
include(cmake/http-parser.cmake)

if(UVHTTP_TLS)
	add_subdirectory(vendor/evt-tls)
endif(UVHTTP_TLS)

add_subdirectory(src)


if (UVHTTP_BUILD_TEST)
    add_subdirectory(test)
endif(UVHTTP_BUILD_TEST)


if (UVHTTP_BUILD_EXAMPLE)
    add_subdirectory(example)
endif(UVHTTP_BUILD_EXAMPLE)