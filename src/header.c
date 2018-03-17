#include <stdlib.h>
#include <string.h>
#include <uvhttp/header.h>

static uv_http_header_t *uv_header_find(uv_http_header_t *head,
                                        const char *field) {
  uv_http_header_t *next = head;
  while (next) {
    // FIXME: make portable This is not prtable
    if (strcasecmp(field, next->field) == 0) {
      return next;
    }
    next = next->next;
  }

  return NULL;
}

uv_http_header_t *uv_http_header_new() {
  uv_http_header_t *h = malloc(sizeof(uv_http_header_t));
  h->next = NULL;
  return h;
}
void uv_http_header_set(uv_http_header_t *head, const char *field,
                        const char *value) {

  uv_http_header_t *h = uv_header_find(head, field);
  int add = 0;
  if (!h) {
    h = head;
    if (strlen(head->field) > 0) {
      h = uv_http_header_new();
      add = 1;
    }
    strcpy(h->field, field);
  }
  strcpy(h->value, value);
  if (add) {
    while (head->next != NULL) {
      head = head->next;
    }
    head->next = h;
  }
}

void uv_http_header_seti(uv_http_header_t *head, const char *field, size_t flen,
                         const char *value, size_t vlen) {
  uv_http_header_t *h = uv_header_find(head, field);
  int add = 0;
  if (!h) {
    h = head;
    if (strlen(head->field) > 0) {
      h = uv_http_header_new();
      add = 1;
    }
    strncpy(h->field, field, flen);
    h->field[flen] = '\0';
  }
  strncpy(h->value, value, vlen);
  h->value[vlen] = '\0';
  if (add) {
    while (head->next != NULL) {
      head = head->next;
    }
    head->next = h;
  }
}

const char *uv_http_header_get(uv_http_header_t *head, const char *field) {
  uv_http_header_t *h = uv_header_find(head, field);
  if (h)
    return h->value;
  return NULL;
}

void uv_http_header_unset(uv_http_header_t **head, const char *field) {
  uv_http_header_t *prev = NULL, *next = *head;

  while (next) {
    if (strcmp(field, next->field) == 0) {
      if (!prev) {
        if (!next->next) {
          memset(next->field, 0, sizeof(next->field));
          memset(next->value, 0, sizeof(next->value));
        } else {
          *head = next->next;
          free(next);
        }
      } else {
        prev->next = next->next;
        free(next);
      }
      break;
    }
    prev = next;
    next = next->next;
  }
}

void uv_http_header_free(uv_http_header_t *head) {
  if (!head)
    return;

  uv_http_header_t *tmp, *next = head->next;
  while (next) {
    tmp = next->next;
    free(next);
    next = tmp;
  }
  free(head);
}

void uv_http_header_append(uv_http_header_t *head, uv_http_header_t *header) {
  while (head->next != NULL) {
    head = head->next;
  }
  head->next = header;
}

int uv_http_header_size(uv_http_header_t *head) {
  int size = 0;
  if (!head)
    return size;
  uv_http_header_foreach(item, head) {
    size += strlen(item->field) + strlen(item->value + 3 /* 1 ':' and a ' ' */);
  }
  size += 2;
  return size;
}