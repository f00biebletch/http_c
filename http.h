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
  char *resource;
  char *version;
  hdr_t *headers;
  char *body;
} http_req_t;

typedef struct http_resp_t {
  char *version;
  char *status;
  hdr_t *headers;
  char *body;
} http_resp_t;

http_req_t *request(const char *p);
http_resp_t *response(http_req_t *h, char *status);
char *response_to_s(http_resp_t *resp);

void dump_request(http_req_t *http);
void free_request(http_req_t *http);
void dump_response(http_resp_t *http);
void free_response(http_resp_t *http);
