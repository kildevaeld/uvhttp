#include "parser.h"
#include "debug.h"
#include "http_p.h"
#include <stdlib.h>
#include <string.h>
#include <uvhttp/http.h>

char *strslice(const char *s, size_t len) {
  char *slice = (char *)malloc(sizeof(char) * (len + 1));
  strncpy(slice, s, len);
  slice[len] = '\0';
  return slice;
}

int on_chunk_header(http_parser *parser) {
  // printf("on chunk header\n");
  return 0;
}
int on_chunk_complete(http_parser *parser) {

  // printf("on chunk complete\n");
  return 0;
}

int on_message_begin(http_parser *parser) { return 0; }
int on_url(http_parser *parser, const char *hdr, size_t length) {
  // debug("url");
  return 0;
}
int on_header_field(http_parser *parser, const char *hdr, size_t length) {
  if (length == -1) {
    return 0;
  }

  http_client_t *client = (http_client_t *)parser->data;
  if (client->callbacks->on_headers) {
    memcpy(client->current_header.field, hdr, length);
    client->current_header.field[length] = '\0';
    // debug("field %s", client->current_header.field);
  }
  return 0;
}
int on_header_value(http_parser *parser, const char *hdr, size_t length) {

  http_client_t *client = (http_client_t *)parser->data;

  if (!client->callbacks->on_headers)
    return 0;

  if (!client->response_header) {
    client->response_header = uv_http_header_new();
  }

  debug("read header %s", client->current_header.field);
  uv_http_header_seti(client->response_header, client->current_header.field,
                      strlen(client->current_header.field), hdr, length);

  return 0;
}
int on_headers_complete(http_parser *parser) {
  http_client_t *client = (http_client_t *)parser->data;

  if (client->callbacks->on_headers)
    client->callbacks->on_headers(client, parser->status_code,
                                  client->response_header);

  if (client->response_header)
    uv_http_header_free(client->response_header);

  client->response_header = NULL;

  return 0;
}

static void on_close(uv_handle_t *h) {
  http_client_t *client = (http_client_t *)h;
  if (client->callbacks->on_finished)
    client->callbacks->on_finished(client);
}

int on_message_complete(http_parser *parser) {
  debug("message complete");
  http_client_t *client = (http_client_t *)parser->data;
  uv_close((uv_handle_t *)client, on_close);

  return 0;
}
int on_body(http_parser *parser, const char *hdr, size_t length) {
  http_client_t *client = (http_client_t *)parser->data;
  debug("read body %lu", length);
  if (client->callbacks->on_data)
    client->callbacks->on_data(client, hdr, length);

  return 0;
}