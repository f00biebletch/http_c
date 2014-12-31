#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CL "content-length"

typedef struct hdr_t {
  char *key;
  char *value;
  struct hdr_t *next;
} hdr_t;

typedef struct http_req_t {
  char *method;
  char *version;
  char *resource;
  hdr_t *headers;
  char *body;
} http_req_t;

http_req_t *parse_http(char *p);
void dump_http(http_req_t *http);
void free_http(http_req_t *http);
