#pragma once
#include <stdbool.h>
#include <uv.h>
#include <uvhttp/request.h>

typedef struct http_client_t http_client_t;

typedef bool (*uv_http_headers_cb)(http_client_t *client, int status,
                                   uv_http_header_t *header);
typedef bool (*uv_http_data_cb)(http_client_t *client, const char *data,
                                size_t len);
typedef void (*uv_http_finished_cb)(http_client_t *client);

typedef void (*uv_http_connect_cb)(http_client_t *client, int status);

typedef void (*uv_http_error_cb)(http_client_t *client, const char *error);

typedef struct http_request_callbacks {
  uv_http_headers_cb on_headers;
  uv_http_data_cb on_data;
  uv_http_finished_cb on_finished;
  uv_http_connect_cb on_connect;
  uv_http_error_cb on_error;

} http_request_callbacks;

struct http_client_t {
  uv_stream_t handle;
  http_request_t *req;
  uv_loop_t *loop;
  void *data;

  http_request_callbacks *callbacks;
  // Private
  bool headers_sent;
  uv_http_header_t current_header;
  uv_http_header_t *response_header;
};

http_client_t *uv_http_create(uv_loop_t *loop, http_request_t *req);

void uv_http_free(http_client_t *);

int uv_http_request(http_client_t *client, http_request_callbacks *callbacks);

int uv_http_request_write(http_client_t *client, uv_buf_t *buf, uv_write_cb cb);

int uv_http_request_end(http_client_t *client);