#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <uv.h>
#include <uvhttp/request.h>
#include <uvhttp/typedefs.h>

/**
 * Create a new http client
 **/
http_client_t *uv_http_create(uv_loop_t *loop, http_request_t *req);

void uv_http_free(http_client_t *);

void uv_http_set_data(http_client_t *, void *);

void *uv_http_get_data(http_client_t *);

http_request_t *uv_http_get_request(http_client_t *);

int uv_http_request(http_client_t *client, http_request_callbacks *callbacks);

int uv_http_request_write(http_client_t *client, uv_buf_t *buf, uv_write_cb cb);

int uv_http_request_end(http_client_t *client);

#ifdef __cplusplus
}
#endif