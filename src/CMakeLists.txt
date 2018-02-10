
set(SOURCES
    http.c
    http_p.c
    header.c
    request.c
    parser.c
    ${PROJECT_SOURCE_DIR}/vendor/http-parser/http_parser.c
)

if(UVHTTP_TLS)
    set(SOURCES
        ${SOURCES}
    )
endif(UVHTTP_TLS)

add_library(uvhttp ${SOURCES})
target_link_libraries(uvhttp uv)

target_include_directories(uvhttp PUBLIC ${PROJECT_SOURCE_DIR}/vendor/http-parser)

if (NOT UVHTTP_BUILD_DEBUG)
    target_compile_definitions(uvhttp PUBLIC NDEBUG=1)
endif(NOT UVHTTP_BUILD_DEBUG)