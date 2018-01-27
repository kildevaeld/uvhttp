#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <uvhttp/header.h>

typedef struct http_client_t http_client_t;

typedef bool (*uv_http_headers_cb)(http_client_t *client, int status,
                                   uv_http_header_t *header);
typedef bool (*uv_http_data_cb)(http_client_t *client, const char *data,
                                size_t len);
typedef void (*uv_http_finished_cb)(http_client_t *client);

typedef void (*uv_http_connect_cb)(http_client_t *client, int status);

typedef void (*uv_http_error_cb)(http_client_t *client, const char *name, const char *error);

typedef struct http_request_callbacks {
  uv_http_headers_cb on_headers;
  uv_http_data_cb on_data;
  uv_http_finished_cb on_finished;
  uv_http_connect_cb on_connect;
  uv_http_error_cb on_error;

} http_request_callbacks;