#include <stdio.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <ev.h>
#include "http.h"

#define PORT_NO 6666
#define BUFFER_SIZE 4096

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

typedef struct client_t {
  int fd;
  ev_io ev_accept;
  ev_io ev_read;
  ev_io ev_write;
  char *raw_req;
  http_req_t *request;
} client_t;

static inline int setnonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) return flags;
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) < 0) return -1;
  return 0;
}

int main(int argc, char **argv)
{
  struct ev_loop *loop = ev_default_loop(0);
  struct sockaddr_in listen_addr;

  memset(&listen_addr, 0, sizeof(listen_addr));
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons(PORT_NO);
  listen_addr.sin_addr.s_addr = INADDR_ANY;


  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("listen failed(socket)");
    return -1;
  }

  int reuseaddr_on = 1;
  if (setsockopt(fd,
		 SOL_SOCKET,
		 SO_REUSEADDR,
		 &reuseaddr_on,
		 sizeof(listen_addr)) == -1) {
    perror("setsockopt failed");
    return -1;
  }

  if (bind(fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
    perror("bind failed");
    return -1;
  }
  if (listen(fd, 5) < 0) {
    perror("listen failed(listen)");
    return -1;
  }
  if (setnonblock(fd) < 0) {
    perror("failed to set server socket to nonblock");
    return -1;
  }

  client_t *main_client = (client_t *)malloc(sizeof(client_t));
  ev_io_init(&main_client->ev_accept, accept_cb, fd, EV_READ);
  ev_io_start(loop, &main_client->ev_accept);

  ev_loop(loop, 0);

  return 0;
}

/* Accept client requests */
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
 client_t *main_client =
   (client_t *)
   (((char *) watcher) - offsetof(struct client_t, ev_accept));

  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int client_fd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);

  if (client_fd < 0) {
    perror("accept error");
    return;
  }

  if (setnonblock(client_fd) < 0) {
    perror("failed to set client socket to nonblock");
    return;
  }

  client_t *client = (client_t *)malloc(sizeof(client_t));
  client->fd = client_fd;
  ev_io_init(&client->ev_read, read_cb, client->fd, EV_READ);
  ev_io_start(loop, &client->ev_read);
}

void read_cb(struct ev_loop *loop, struct ev_io *w, int revents){
  char buffer[BUFFER_SIZE];

  if(!(EV_READ & revents)) {
    ev_io_stop(EV_A_ w);
    return;
  }

  client_t *client =
    (client_t *)
    (((char *) w) - offsetof(struct client_t, ev_read));

  int total = 0;
  int len = 0;
  client->raw_req = NULL;
  do {
    len = read(client->fd, &buffer, BUFFER_SIZE);
    total += len;
    if (len < BUFFER_SIZE) buffer[len] = '\0';
    if (client->raw_req == NULL) {
      client->raw_req = malloc(len+1);
      memcpy(client->raw_req, buffer, len);
    } else {
      client->raw_req = realloc(client->raw_req, total + 1);
      memcpy(client->raw_req + total - len, buffer, len);
    }
  } while (len == BUFFER_SIZE);

  http_req_t *req = request(client->raw_req);
  dump_request(req);
  client->request = req;
  memset(buffer, 0, BUFFER_SIZE);

  ev_io_stop(EV_A_ w);
  ev_io_init(&client->ev_write, write_cb, w->fd, EV_WRITE);
  ev_io_start(loop, &client->ev_write);
}

void write_cb(struct ev_loop *loop, struct ev_io *w, int revents){

  if (!(revents & EV_WRITE)) {
    ev_io_stop(EV_A_ w);
    return;
  }
  client_t *client =
    (client_t *)
    (((char *) w) - offsetof(struct client_t, ev_write));
  http_req_t *req = client->request;

  dump_request(req);
  http_resp_t *resp = response(req, "200 OK");
  dump_response(resp);
  
  char *data = response_to_s(resp);

  send(client->fd, data, strlen(data), 0);

  free(client->raw_req);
  client->raw_req = NULL;
  free_request(req);
  free_response(resp);
  free(data);
  close(client->fd);

  ev_io_stop(EV_A_ w);
}
