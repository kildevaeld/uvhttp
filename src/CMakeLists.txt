
set(SOURCES
    client.c
    header.c
    request.c
    parser.c
    client_p.c
)

add_library(uvhttp ${SOURCES})
target_link_libraries(uvhttp uv http_parser)

target_include_directories(uvhttp PUBLIC ${PROJECT_ROOT}/vendor/http-parser)

if (NOT UVHTTP_BUILD_DEBUG)
    target_compile_definitions(uvhttp PUBLIC NDEBUG=1)
endif(NOT UVHTTP_BUILD_DEBUG)