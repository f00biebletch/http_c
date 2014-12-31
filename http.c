#include "http.h"

static void parse_req(char **p, http_req_t *http);
static void read_line(char **p);
static void parse_hdrs(char **p, http_req_t *http);
static int get_len(http_req_t *http);
static void parse_body(char *p, http_req_t *http);
static void parse_hdr(char *buf, hdr_t *hdr);
static hdr_t *do_parse_hdrs(char **p);
static void free_hdrs(hdr_t *h);
static void dump_hdrs(hdr_t *h);
static void consume(char **src, char *dest);

int main(int argc, char **argv) {
  char *strs[] = {
    "GET HTTP/1.1 /foo/bar\r\ncontent-lenGTH: 10\r\ndork:2\r\n\r\n0123456789",
    "GET HTTP/1.1 /foo/bar\r\ndork:bigTIMES\r\n\r\n",
    "POST HTTP/1.1 /foo/bar/quux/bletch.jsp\r\ncontent-lenGTH: 2\r\nDORK:2\r\n\r\n69",
    "GET HTTP/1.1 /foo/bar\r\n\r\n\r\n"
  };

  for (int i=0; i<4; i++) {
    http_req_t *http = parse_http(strs[i]);
    dump_http(http);
    free_http(http);
  }
}

http_req_t *parse_http(char *buf) {

  http_req_t *http = (http_req_t *)malloc(sizeof(http_req_t));
  char *p = buf;

  parse_req(&p, http);
  parse_hdrs(&p, http);
  read_line(&p);
  parse_body(p, http);

  return http;
}

void dump_http(http_req_t *http) {
  printf("method: %s\n", http->method);
  printf("version: %s\n", http->version);
  printf("resource: %s\n", http->resource);
  dump_hdrs(http->headers);
  printf("body: %s\n", http->body);
}

void free_http(http_req_t *http) {
  free(http->method);
  free(http->version);
  free(http->resource);
  free(http->body);
  free_hdrs(http->headers);
  free(http);
}

static void parse_req(char **p, http_req_t *http) {

  char *m = (char *)malloc(32);
  consume(p, m);
  (*p)++;

  char *v = (char *)malloc(32);
  consume(p, v);
  (*p)++;

  char *r = (char *)malloc(2048);
  consume(p, r);
  *p+=2;

  http->method = m;
  http->version = v;
  http->resource = r;
}

static void consume(char **src, char *dest) {
  while (**src != '\0' && **src != ' ' && **src != '\r') {
    *dest = **src;
    dest++;
    (*src)++;
  }
}

static void read_line(char **p) {
  while (**p != '\0' && **p != '\r') (*p)++;
  *p+=2;
}

static void parse_hdrs(char **p, http_req_t *http) {
  http->headers = do_parse_hdrs(p);
}

static hdr_t *do_parse_hdrs(char **p) {
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

static void parse_hdr(char *p, hdr_t *hdr) {

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

static void free_hdrs(hdr_t *h) {
  hdr_t *cur = h;
  while (cur) {
    hdr_t *tmp = cur->next;
    free(cur->key);
    free(cur->value);
    free(cur);
    cur = tmp;
  }
}

static void dump_hdrs(hdr_t *h) {
  hdr_t *cur = h;
  while (cur) {
    printf("%s : %s\n", cur->key, cur->value);
    cur = cur->next;
  }
}

static void parse_body(char *p, http_req_t *http) {
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
    }
    cur = cur->next;
  }
  return len;
}

