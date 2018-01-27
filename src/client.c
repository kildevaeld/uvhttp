#include "client_p.h"
#include "debug.h"
#include "parser.h"
#include <http_parser.h>
#include <stdlib.h>
#include <uv.h>
#include <uvhttp/client.h>

/*static http_parser_settings parser_settings = {
    .on_message_begin = on_message_begin,
    .on_url = on_url,
    .on_header_field = on_header_field,
    .on_header_value = on_header_value,
    .on_headers_complete = on_headers_complete,
    .on_message_complete = on_message_complete,
    .on_body = on_body,
    .on_chunk_header = on_chunk_header,
    .on_chunk_complete = on_chunk_complete};

#define UVERR(r, msg)                                                          \
  fprintf(stderr, "%s: [%s(%d): %s]\n", msg, uv_err_name((r)), r,              \
          uv_strerror((r)));
*/
static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  buf->base = malloc(size);
  buf->len = size;
}

/*static void on_resolved(uv_getaddrinfo_t *resolver, int status,
                        struct addrinfo *res);

static void on_connect(uv_connect_t *connect, int status);

static void on_req_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);

static inline bool is_ip(const char *ipAddress);

static inline bool is_chunked(uv_http_header_t *headers);

static int maybe_write_headers(http_client_t *client);

static void on_write_end(uv_write_t *req, int status);

static int uv_http_req_write_headers(uv_write_t *write, uv_stream_t *stream,
                                     http_request_t *req, uv_write_cb cb);*/

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

// PRIVATES
/*
static inline bool is_ip(const char *ipAddress) {
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
  return result != 0;
}

static inline bool is_chunked(uv_http_header_t *headers) {

  if (headers) {
    const char *t = uv_http_header_get(headers, "transfer-encoding");
    if (t && strcmp(t, "chunked") == 0) {
      return true;
    }
  }
  return false;
}

static void on_write_end(uv_write_t *req, int status) {
  if (status == -1) {
    fprintf(stderr, "error on_write_end");
    goto on_write_end_end;
  }
  if (req->data) {
    // http_client_t *client = req->data;

    // cb(req, status);
  }

on_write_end_end:
  free(req);
}

static int maybe_write_headers(http_client_t *client) {

  if (client->headers_sent) {
    return 0;
  }

  uv_write_t *w = malloc(sizeof(uv_write_t));
  client->headers_sent = true;
  w->data = client;
  debug("sending headers to: %s", client->req->host);
  return uv_http_req_write_headers(
      w, &client->handle, (http_request_t *)client->req, (uv_write_cb)free);
}

static void on_resolved(uv_getaddrinfo_t *resolver, int status,
                        struct addrinfo *res) {
  if (status < 0) {
    fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status));
    free(resolver);
    return;
  }

  struct addrinfo *tmp = res;

  uv_connect_t *connect_req = malloc(sizeof(uv_connect_t));

  http_client_t *client = resolver->data;
  connect_req->data = client->callbacks->on_connect;

  uv_tcp_init(resolver->loop, (uv_tcp_t *)&client->handle);

  // Set host header
  if (client->req->headers) {
    if (!uv_http_header_get(client->req->headers, "host")) {
      uv_http_header_set(client->req->headers, "host", client->req->host);
    }
  }

  int rc;
  char str[INET_ADDRSTRLEN];

  while (tmp) {

    inet_ntop(AF_INET, &(tmp->ai_addr), str, INET_ADDRSTRLEN);
    debug("connecting to %s on %s", client->req->host, str);
    rc = uv_tcp_connect(connect_req, (uv_tcp_t *)client,
                        (const struct sockaddr *)tmp->ai_addr, on_connect);
    if (!rc) {
      break;
    }

    debug("could not connect %s", uv_err_name(rc));
    tmp = tmp->ai_next;
  }

  uv_freeaddrinfo(res);
  free(resolver);
}

static void on_connect(uv_connect_t *connect, int status) {
  if (status < 0) {
    fprintf(stderr, "connect callback error %s\n", uv_err_name(status));
    goto connect_end;
  }

  http_client_t *client = (http_client_t *)connect->handle;
  debug("connected to %s:%i%s", client->req->host, client->req->port,
        client->req->path ? client->req->path : "/");

  if (connect->data) {
    uv_http_connect_cb cb = connect->data;
    cb(client, status);
    goto connect_end;
  }

connect_end:
  free(connect);
}

static void on_req_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
  size_t parsed;
  http_client_t *handle = (http_client_t *)tcp;
  http_request_t *req = handle->req;
  req->parser.data = handle;

  if (nread == UV_EOF) {
    debug("eof");
    http_parser_execute(&req->parser, &parser_settings, buf->base, 0);
    uv_close((uv_handle_t *)tcp, NULL);
  } else if (nread > 0) {

    parsed =
        http_parser_execute(&req->parser, &parser_settings, buf->base, nread);

    if (parsed < nread) {
      const char *nam = http_errno_name(HTTP_PARSER_ERRNO(&req->parser));
      const char *err = http_errno_description(HTTP_PARSER_ERRNO(&req->parser));

      log_err("parsing http req  %s: %s", nam, err);
      uv_close((uv_handle_t *)tcp, NULL);
    }
  } else {
    UVERR((int)nread, "reading req req");
  }
  if (buf->base)
    free(buf->base);
}

static int write_method(http_request_t *req, char *buf) {
  int i = 0;
  switch (req->method) {
  case HTTP_GET:
    memcpy(buf, "GET", 3);
    i += 3;
    break;
  case HTTP_POST:
    memcpy(buf, "POST", 4);
    i += 4;
    break;
  case HTTP_DELETE:
    memcpy(buf, "DELETE", 6);
    i += 6;
  case HTTP_HEAD:
    memcpy(buf, "HEAD", 4);
    i += 4;
  case HTTP_OPTIONS:
    memcpy(buf, "OPTIONS", 7);
    i += 7;
  default:
    return -1;
  }

  return i;
}

static int write_request(http_request_t *req, char *buf) {

  int i = write_method(req, buf);

  int major = 1, minor = 1;
  if (req->major > -1)
    major = req->major;
  if (req->minor > -1)
    minor = req->minor;

  const char *path = req->path;
  if (!path)
    path = "/";

  int ret = sprintf(buf + i, " %s HTTP/%d.%d\r\n", path, major, minor);
  if (ret < 0) {
    return -1;
  }
  i += ret;

  if (req->headers) {
    uv_http_header_foreach(h, req->headers) {
      int ret = sprintf(buf + i, "%s: %s\r\n", h->field, h->value);
      i += ret;
    }
  }

  memcpy(buf + i, "\r\n", 2);
  i += 2;

  return i;
}*/

/*static int uv_http_req_write_headers(uv_write_t *write, uv_stream_t *stream,
                                     http_request_t *req, uv_write_cb cb) {
  const char *path = req->path;
  if (!path)
    path = "/";
  char message[uv_http_header_size(req->headers) + strlen(path) + 20];
  uv_buf_t buf; // = uv_buf_init(message, write_request(req, message));
  buf.len = write_request(req, message);
  buf.base = message;

  return uv_write(write, stream, &buf, 1, on_write_end);
}*/