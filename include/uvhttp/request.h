#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <uv.h>
#include <uvhttp/header.h>
#include <uvhttp/typedefs.h>

typedef struct http_request_t {
  UV_HTTP_REQUEST_DEF;
  const char *host;
  int port;
} http_request_t;

bool uv_http_request_init(http_request_t *req, uv_http_method_t method,
                          const char *address);

void uv_http_request_free(http_request_t *req);

#ifdef __cplusplus
}
#endif