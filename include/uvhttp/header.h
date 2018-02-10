#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define UV_HTTP_HEADER_MAX 500

typedef struct uv_http_header_s {
  char field[UV_HTTP_HEADER_MAX];
  char value[UV_HTTP_HEADER_MAX];
  struct uv_http_header_s *next;
} uv_http_header_t;

#define uv_http_header_foreach(item, list)                                     \
  for (uv_http_header_t * (item) = (list); (item); (item) = (item)->next)

/**
 * Create a new header
 * The user is responsible for freeing it with a call to uv_http_header_free
 */
uv_http_header_t *uv_http_header_new();
/**
 * Set a header
 */
void uv_http_header_set(uv_http_header_t *header, const char *field,
                        const char *value);

void uv_http_header_seti(uv_http_header_t *head, const char *field, size_t flen,
                         const char *value, size_t vlen);

const char *uv_http_header_get(uv_http_header_t *header, const char *field);
void uv_http_header_unset(uv_http_header_t **header, const char *field);
void uv_http_header_free(uv_http_header_t *header);

/**
 * Append a header to the header list */
void uv_http_header_append(uv_http_header_t *head, uv_http_header_t *header);
int uv_http_header_size(uv_http_header_t *head);

#ifdef __cplusplus
}
#endif
