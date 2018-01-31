#pragma once
#include "defs.h"
#include <stdbool.h>
#include <uv.h>
#include <uvhttp/header.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct http_request_t {
  UV_HTTP_REQUEST_DEF;
  const char *host;
  int port;
} http_request_t;

bool uv_http_request_init(http_request_t *req, uv_http_method_t method,
                          const char *address);

#ifdef __cplusplus
}
#endif