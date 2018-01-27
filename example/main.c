#include <stdio.h>
#include <uvhttp/client.h>
#include <uvhttp/request.h>

static bool on_headers(http_client_t *client, int status,
                       uv_http_header_t *header) {
  printf("Conncted %i\n", status);
  uv_http_header_foreach(h, header) { printf("%s: %s\n", h->field, h->value); }

  return false;
}

static bool on_data(http_client_t *client, const char *data, size_t size) {
  printf("data %lu \n", size);

  FILE *file = client->data;

  size_t out = fwrite(data, sizeof(char), size, file);

  if (out != size) {
    return false;
  }

  return true;
}

static void on_connect(http_client_t *client, int status) {
  printf("connect\n");
  uv_buf_t buf;
  buf.base = "Hello, World";
  buf.len = strlen(buf.base);

  uv_http_request_write(client, &buf, NULL);

  uv_http_request_end(client);
}

static void on_finished(http_client_t *client) {
  printf("done\n");
  uv_http_free(client);
}

int main() {

  FILE *f;

  f = fopen("response.txt", "w");
  if (!f) {
    fprintf(stderr, "could not open file\n");
    return 1;
  }

  uv_loop_t *loop = uv_default_loop();
  http_request_callbacks cb = {.on_connect = on_connect,
                               .on_headers = on_headers,
                               .on_finished = on_finished,
                               .on_data = on_data};
  http_request_t req;
  uv_http_request_init(&req, HTTP_GET, "http://localhost:3000");
  http_client_t *client = uv_http_create(loop, &req);

  client->data = f;

  uv_http_request(client, &cb);

  uv_run(loop, UV_RUN_DEFAULT);

  fclose(f);

  // sleep(10);

  return 0;
}