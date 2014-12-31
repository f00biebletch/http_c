#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CL "content-length"

typedef struct pair_t {
  char *key;
  char *val;
} pair_t;

typedef struct hdr_t {
  pair_t *val;
  struct hdr_t *next;
} hdr_t;

typedef struct http_req_t {
  char *method;
  char *resource;
  hdr_t *headers;
  char *body;
} http_req_t;

http_req_t *parse_http(char *p);

void parse_req(char **p, http_req_t *http);
void read_line(char **p);
void parse_hdrs(char **p, http_req_t *http);
void dump_http(http_req_t *http);
void free_http(http_req_t *http);

int get_len(http_req_t *http);

void parse_body(char *p, http_req_t *http);
hdr_t *do_parse_hdrs(char **p);
pair_t *parse_pair(char *s);
void free_hdrs(hdr_t *h);
void dump_hdrs(hdr_t *h);

int main(int argc, char **argv) {
  /*  pair_t *e = parse_pair("content-length: 666");
  printf("got %s to %s\n", e->key, e->val);
  free(e);

  char *str = "conTENT-length: 666\r\ndork:2\r\n";
  hdr_t *hdrs = do_parse_hdrs(str);
  dump_hdrs(hdrs);
  free_hdrs(hdrs);
  */
  char *str2 = "GET HTTP/1.1 /foo/bar\r\ncontent-lenGTH: 10\r\ndork:2\r\n\r\n0123456789";
  http_req_t *http = parse_http(str2);

  dump_http(http);
  free_http(http);
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

void parse_req(char **p, http_req_t *http) {

  char *method = (char *)malloc(32);
  char *m = method;
  while (**p != '\0' && **p != ' ') {
    *m = **p;
    m++;
    (*p)++;
  }
  (*p)++;

  char *resource = (char *)malloc(2048);
  char *r = resource;
  while (**p != '\0' && **p != '\r') {
    *r = **p;
    r++;
    (*p)++;
  }
  *p+=2;

  http->method = method;
  http->resource = resource;
}

void read_line(char **p) {
  while (**p != '\0' && **p != '\r') (*p)++;
  *p+=2;
}

void parse_hdrs(char **p, http_req_t *http) {
  http->headers = do_parse_hdrs(p);
}

hdr_t *do_parse_hdrs(char **p) {
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
  hdr->val = parse_pair(line);
  hdr->next = do_parse_hdrs(p);

  return hdr;
}

pair_t *parse_pair(char *buf) {
  char *line = strdup(buf);
  char *key = strsep(&line, ":");
  char *val = strsep(&line, ":");

  char *k = key;
  for ( ; *k; ++k) *k = tolower(*k);

  pair_t *entry = (pair_t *) malloc(sizeof(pair_t));
  entry->key = key;
  entry->val = val;

  free(line);

  return entry;
}

void free_http(http_req_t *http) {
  free(http->method);
  free(http->resource);
  free(http->body);
  free_hdrs(http->headers);
  free(http);
}

void free_hdrs(hdr_t *h) {
  hdr_t *cur = h;
  while (cur) {
    hdr_t *tmp = cur->next;
    free(cur->val);
    free(cur);
    cur = tmp;
  }
}

void dump_hdrs(hdr_t *h) {
  hdr_t *cur = h;
  while (cur) {
    printf("%s :: %s\n", cur->val->key, cur->val->val);
    cur = cur->next;
  }
}

void dump_http(http_req_t *http) {
  printf("method: %s\n", http->method);
  printf("resource: %s\n", http->resource);
  dump_hdrs(http->headers);
  printf("body: %s\n", http->body);
}

void parse_body(char *p, http_req_t *http) {
  int len = get_len(http);
  int i = 0;
  http->body = (char *) malloc(len+1);
  memcpy(http->body, p, len);
}

int get_len(http_req_t *http) {
  hdr_t *cur = http->headers;
  int len = 0;
  while (cur) {
    if (strcmp(CL, cur->val->key) == 0) {
      len = atoi(cur->val->val);
    }
    cur = cur->next;
  }
  return len;
}

