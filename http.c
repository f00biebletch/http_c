#include "http.h"

static void parse_req(const char **p, http_req_t *http);
static void skip_crlf(const char **p);
static void parse_hdrs(const char **p, http_req_t *http);
static int get_len(http_req_t *http);
static void parse_body(const char *p, http_req_t *http);
static void parse_hdr(const char *buf, hdr_t *hdr);
static hdr_t *do_parse_hdrs(const char **p);
static void free_hdrs(hdr_t *h);
static void dump_hdrs(hdr_t *h);
static void consume(const char **src, char *dest);
static hdr_t *cp_hdrs(hdr_t *s);

int main(int argc, char **argv) {
  const char *strs[] = {
    "GET /foo/bar HTTP/1.1\r\ncontent-lenGTH: 10\r\ndork:2\r\n\r\n0123456789",
    "GET /foo/bar HTTP/1.1\r\ndork:bigTIMES\r\n\r\n",
    "POST /foo/bar/quux/bletch.jsp HTTP/1.1\r\ncontent-lenGTH: 2\r\nDORK:2\r\n\r\n69",
    "GET /foo/bar HTTP/1.1\r\n\r\n\r\n"
  };

  for (int i=0; i<4; i++) {
    http_req_t *http = request(strs[i]);
    http_resp_t *resp = response(http);
    dump_request(http);
    dump_response(resp);
    free_request(http);
    free_response(resp);
  }
}

http_resp_t *response(http_req_t *req) {
  http_resp_t *resp = (http_resp_t *)malloc(sizeof(http_resp_t));
  resp->version = strdup(req->version);
  resp->status = (char *)malloc(64);
  strcpy(resp->status, "200 OK");
  resp->headers = cp_hdrs(req->headers);
  resp->body = strdup(req->body);
  
  return resp;
}

http_req_t *request(const char *buf) {

  http_req_t *http = (http_req_t *)malloc(sizeof(http_req_t));
  const char *p = buf;

  parse_req(&p, http);
  parse_hdrs(&p, http);
  skip_crlf(&p);
  parse_body(p, http);

  return http;
}

void dump_request(http_req_t *http) {
  printf("method: %s\n", http->method);
  printf("version: %s\n", http->version);
  printf("resource: %s\n", http->resource);
  dump_hdrs(http->headers);
  printf("body: %s\n", http->body);
}

void dump_response(http_resp_t *http) {
  printf("version: %s\n", http->version);
  printf("status: %s\n", http->status);
  dump_hdrs(http->headers);
  printf("body: %s\n", http->body);
}

void free_request(http_req_t *http) {
  free(http->method);
  http->method = NULL;
  free(http->version);
  http->version = NULL;
  free(http->resource);
  http->resource = NULL;
  free(http->body);
  http->body = NULL;
  free_hdrs(http->headers);
  free(http);
  http = NULL;
}

void free_response(http_resp_t *http) {
  free(http->version);
  http->version = NULL;
  free(http->body);
  http->body = NULL;
  free(http->status);
  http->status = NULL;
  free(http->headers);
  free(http);
  http = NULL;
}

static void parse_req(const char **p, http_req_t *http) {

  char *m = (char *)malloc(32);
  consume(p, m);
  (*p)++;

  char *r = (char *)malloc(2048);
  consume(p, r);
  (*p)++;

  char *v = (char *)malloc(32);
  consume(p, v);
  *p+=2;

  http->method = m;
  http->version = v;
  http->resource = r;
}

static void consume(const char **src, char *dest) {
  while (**src != '\0' && **src != ' ' && **src != '\r') {
    *dest = **src;
    dest++;
    (*src)++;
  }
}

static void skip_crlf(const char **p) {
  while (**p != '\0' && **p != '\r') (*p)++;
  *p+=2;
}

static void parse_hdrs(const char **p, http_req_t *http) {
  http->headers = do_parse_hdrs(p);
}

static hdr_t *do_parse_hdrs(const char **p) {
  if (**p == '\0' || **p == '\r' || **p == '\n') return NULL;

  char line[512] = {0};
  char *l = &line[0];
  while (**p != '\0' && **p != '\r') {
    *l = **p;
    l++;
    (*p)++;
  }
  *p+=2;

  hdr_t *hdr = (hdr_t *) malloc(sizeof(hdr_t));
  parse_hdr(line, hdr);
  hdr->next = do_parse_hdrs(p);

  return hdr;
}

static void parse_hdr(const char *p, hdr_t *hdr) {

  char *key = (char *)malloc(64);
  char *k = key;
  while (*p != '\0' && *p != ':') {
    *k++ = tolower(*p);
    p++;
  }
  p++;

  char *val = (char *)malloc(64);
  char *v = val;
  while (*p != '\0') *v++ = *p++;

  hdr->key = key;
  hdr->value = val;
}

static hdr_t *cp_hdrs(hdr_t *s) {
  if (s == NULL) return NULL;

  hdr_t *d = (hdr_t *)malloc(sizeof(hdr_t));
  d->key = strdup(s->key);
  d->value = strdup(s->value);
  d->next = cp_hdrs(s->next);
  return d;
}

static void free_hdrs(hdr_t *h) {
  hdr_t *cur = h;
  while (cur) {
    hdr_t *tmp = cur->next;
    free(cur->key);
    cur->key = NULL;
    free(cur->value);
    cur->value = NULL;
    free(cur);
    cur = NULL;
    cur = tmp;
  }
  h = NULL;
}

static void dump_hdrs(hdr_t *h) {
  hdr_t *cur = h;
  while (cur) {
    printf("==> %s : %s\n", cur->key, cur->value);
    cur = cur->next;
  }
}

static void parse_body(const char *p, http_req_t *http) {
  int len = get_len(http);
  http->body = (char *) malloc(len+1);
  memcpy(http->body, p, len);
}

static int get_len(http_req_t *http) {
  hdr_t *cur = http->headers;
  int len = 0;
  while (cur) {
    if (strcmp(CL, cur->key) == 0) {
      len = atoi(cur->value);
      break;
    }
    cur = cur->next;
  }
  return len;
}

