
set(SOURCES
    client.c
    header.c
    request.c
    parser.c
    client_p.c
)

add_library(uvhttp ${SOURCES})
target_link_libraries(uvhttp uv http_parser)