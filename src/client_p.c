#include "client_p.h"
#include "debug.h"
#include "parser.h"
#include <stdlib.h>
#include <uvhttp/client.h>

static http_parser_settings parser_settings = {
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

bool is_ip(const char *ipAddress) {
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
  return result != 0;
}

bool is_chunked(uv_http_header_t *headers) {

  if (headers) {
    const char *t = uv_http_header_get(headers, "transfer-encoding");
    if (t && strcmp(t, "chunked") == 0) {
      return true;
    }
  }
  return false;
}

void on_write_end(uv_write_t *req, int status) {
  if (status == -1) {
    fprintf(stderr, "error on_write_end");
    goto on_write_end_end;
  }
  if (req->data) {
     uv_write_cb cb = req->data;
     cb(req, status);
     return;
  }

on_write_end_end:
  free(req);
}

int uv_http_req_write_headers(uv_write_t *write, uv_stream_t *stream,
                              http_request_t *req, uv_write_cb cb) {
  const char *path = req->path;
  if (!path)
    path = "/";
  char message[uv_http_header_size(req->headers) + strlen(path) + 20];
  uv_buf_t buf; 
  buf.len = write_request(req, message);
  buf.base = message;

  char m[buf.len+1];
  memcpy(m, buf.base, buf.len);
  m[buf.len] = '\0';
  
  write->data = cb;
  return uv_write(write, stream, &buf, 1, on_write_end);
}

int maybe_write_headers(http_client_t *client) {

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

void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
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

void on_connect(uv_connect_t *connect, int status) {
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

void on_req_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
  size_t parsed;
  http_client_t *handle = (http_client_t *)tcp;
  http_request_t *req = handle->req;
  req->parser.data = handle;
    
  if (nread == UV_EOF) {
    debug("reached EOF");
    http_parser_execute(&req->parser, &parser_settings, buf->base, 0);
    //uv_close((uv_handle_t *)&handle->handle, NULL);
  } else if (nread > 0) {

    parsed =
        http_parser_execute(&req->parser, &parser_settings, buf->base, nread);

    if (parsed < nread) {
      const char *nam = http_errno_name(HTTP_PARSER_ERRNO(&req->parser));
      const char *err = http_errno_description(HTTP_PARSER_ERRNO(&req->parser));

      log_err("parsing http req  %s: %s", nam, err);
      if (handle->callbacks->on_error) 
        handle->callbacks->on_error(handle, nam, err);
      uv_close((uv_handle_t *)tcp, NULL);
    }
  } else {
    UVERR((int)nread, "reading req req");
  }
  if (buf->base)
    free(buf->base);
}

int write_method(http_request_t *req, char *buf) {
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

int write_request(http_request_t *req, char *buf) {

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
}