#pragma once
#include <uv.h>
#include <uvhttp/request.h>
#include <uvhttp/typedefs.h>

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

void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res);

void on_connect(uv_connect_t *connect, int status);

void on_req_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);

bool is_ip(const char *ipAddress);

bool is_chunked(uv_http_header_t *headers);

int maybe_write_headers(http_client_t *client);

void on_write_end(uv_write_t *req, int status);

int uv_http_req_write_headers(uv_write_t *write, uv_stream_t *stream,
                              http_request_t *req, uv_write_cb cb);

int write_request(http_request_t *req, char *buf);