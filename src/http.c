#include "debug.h"
#include "http_p.h"
#include "parser.h"
#include <http_parser.h>
#include <stdlib.h>
#include <uv.h>
#include <uvhttp/http.h>

static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  buf->base = malloc(size);
  buf->len = size;
}

http_client_t *uv_http_create(uv_loop_t *loop, http_request_t *req) {
  http_client_t *client = malloc(sizeof(http_client_t));
  if (!client)
    return NULL;

  client->data = NULL;
  client->headers_sent = false;
  client->loop = loop;
  client->req = req;
  client->response_header = NULL;

  return client;
}

void uv_http_free(http_client_t *client) {
  if (!client)
    return;

  free(client);
}

void uv_http_set_data(http_client_t *client, void *data) {
  client->data = data;
}

void *uv_http_get_data(http_client_t *client) { return client->data; }

http_request_t *uv_http_get_request(http_client_t *client) {
  return client->req;
}

int uv_http_request(http_client_t *client, http_request_callbacks *callbacks) {
  // TODO: Handle IP6
  client->callbacks = callbacks;
  http_request_t *req = client->req;

  if (is_ip(req->host)) {
    uv_tcp_init(client->loop, (uv_tcp_t *)&client->handle);

    struct sockaddr_in req_addr;

    uv_ip4_addr(req->host, req->port, &req_addr);

    uv_connect_t *con = malloc(sizeof(uv_connect_t));
    con->data = callbacks->on_connect;

    return uv_tcp_connect(con, (uv_tcp_t *)client,
                          (const struct sockaddr *)&req_addr, on_connect);
  } else {
    struct addrinfo hints;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    uv_getaddrinfo_t *resolver = malloc(sizeof(uv_getaddrinfo_t));

    resolver->data = client;

    char buf[20];
    sprintf(buf, "%i", req->port);
    debug("resolving address: %s", req->host);
    return uv_getaddrinfo(client->loop, resolver, on_resolved, req->host, buf,
                          &hints);
  }

  return 0;
}

int uv_http_request_write(http_client_t *client, uv_buf_t *buf,
                          uv_write_cb cb) {

  int rc;
  if ((rc = maybe_write_headers(client)) != 0)
    return rc;

  uv_http_method_t m = client->req->method;
  if (m != HTTP_POST && m != HTTP_PUT) {
    return 220;
  }
  uv_http_header_t *headers = client->req->headers;
  bool isc = is_chunked(headers);

  uv_write_t *write = malloc(sizeof(uv_write_t));
  write->data = cb;

  if (isc) {
    char str[18 + buf->len + 2];
    int i = sprintf(str, "%x\r\n", (int)buf->len);
    int l = i + buf->len + 2;
    memcpy(str + i, buf->base, buf->len);
    str[l - 2] = '\r';
    str[l - 1] = '\n';
    uv_buf_t buffer = uv_buf_init(str, l);
    debug("write chunk");
    return uv_write(write, (uv_stream_t *)client, &buffer, 1, on_write_end);
  }
  debug("write body");
  return uv_write(write, (uv_stream_t *)client, buf, 1, on_write_end);
}

int uv_http_request_end(http_client_t *client) {
  int rc;
  if ((rc = maybe_write_headers(client)) != 0)
    return rc;

  uv_http_method_t m = client->req->method;
  if (m == HTTP_POST || m == HTTP_PUT) {
    uv_http_header_t *headers = client->req->headers;
    bool isc = is_chunked(headers);

    if (isc) {
      uv_buf_t buf;
      buf.base = "0\r\n\r\n";
      buf.len = 5;
      uv_write_t *write = malloc(sizeof(uv_write_t));
      write->data = NULL;
      debug("write trailing chunk\n");
      int rc = uv_write(write, (uv_stream_t *)client, &buf, 1, on_write_end);
      if (rc != 0) {
        free(write);
        return rc;
      }
    }
  }
  debug("start reading");
  return uv_read_start((uv_stream_t *)client, alloc_cb, on_req_read);
}
