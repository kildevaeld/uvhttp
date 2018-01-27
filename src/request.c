#include <http_parser.h>
#include <stdlib.h>
#include <uvhttp/request.h>

static inline char *get_field(struct http_parser_url *parser, const char *url,
                              int field) {
  if ((parser->field_set & (1 << field)) != 0) {
    size_t len = parser->field_data[field].len;
    char *data = malloc(sizeof(char) * len + 1);
    memcpy(data, url + parser->field_data[field].off,
           parser->field_data[field].len);
    data[len] = '\0';
    return data;
  }

  return NULL;
}

bool uv_http_request_init(http_request_t *req, uv_http_method_t method,
                          const char *address) {

  req->method = method;

  struct http_parser_url url;
  http_parser_url_init(&url);

  int result = http_parser_parse_url(address, strlen(address),
                                     method == HTTP_CONNECT, &url);
  if (result != 0) {
    printf("Parse error : %d\n", result);
    return false;
  }

  req->port = url.port;
  req->host = get_field(&url, address, UF_HOST);
  req->path = get_field(&url, address, UF_PATH);
  req->headers = NULL;
  req->major = 1;
  req->minor = 1;

  http_parser_init(&req->parser, HTTP_RESPONSE);

  if (!req->host) {
    return false;
  }

  return true;
}