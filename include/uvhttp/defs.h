#pragma once
#include <http_parser.h>

typedef enum http_method uv_http_method_t;

#define UV_HTTP_REQUEST_DEF                                                    \
  uv_http_method_t method;                                                     \
  const char *path;                                                            \
  uv_http_header_t *headers;                                                   \
  int major;                                                                   \
  int minor;                                                                   \
  http_parser parser;                                                          \
  void *data;                                                                  \
  uv_loop_t *loop;