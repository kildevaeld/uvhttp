
set(HTTPPARSERDIR ${PROJECT_SOURCE_DIR}/vendor/http-parser)

include_directories(${HTTPPARSERDIR})

add_library(http_parser STATIC
  ${HTTPPARSERDIR}/http_parser.c
)