#include <stdio.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ev.h>
#include "http.h"

// taken from http://codefundas.blogspot.com/2010/09/create-tcp-echo-server-using-libev.html
#define PORT_NO 6666
#define BUFFER_SIZE 4096

int total_clients = 0;  // Total number of connected clients

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

typedef struct client_t {
  int fd;
  ev_io ev_accept;
  ev_io ev_read;
  ev_io ev_write;
  char *buf;
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
  int sd;
  struct sockaddr_in addr;
  int addr_len = sizeof(addr);
  struct ev_io w_accept;

  // Create server socket
  if( (sd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("socket error");
    return -1;
  }

  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_NO);
  addr.sin_addr.s_addr = INADDR_ANY;

  // Bind socket to address
  if (bind(sd, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
    perror("bind error");
  }

  // Start listing on the socket
  if (listen(sd, 2) < 0) {
    perror("listen error");
    return -1;
  }

  // Initialize and start a watcher to accepts client requests
  ev_io_init(&w_accept, accept_cb, sd, EV_READ);
  ev_io_start(loop, &w_accept);

  // Start infinite loop
  while (1) {
    ev_loop(loop, 0);
  }

  return 0;
}

/* Accept client requests */
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_sd;
  struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));

  if(EV_ERROR & revents)
    {
      perror("got invalid event");
      return;
    }

  // Accept client request
  client_sd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);

  if (client_sd < 0)
    {
      perror("accept error");
      return;
    }

  total_clients ++; // Increment total_clients count
  printf("Successfully connected with client.\n");
  printf("%d client(s) connected.\n", total_clients);

  // Initialize and start watcher to read client requests
  ev_io_init(w_client, read_cb, client_sd, EV_READ);
  ev_io_start(loop, w_client);
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents){
  char buffer[BUFFER_SIZE];
  ssize_t read;

  if(EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }

  read = recv(watcher->fd, buffer, BUFFER_SIZE, 0);
  if(read < 0) {
    perror("read error");
    return;
  }

  if(read == 0) {
    // Stop and free watchet if client socket is closing
    ev_io_stop(loop,watcher);
    free(watcher);
    perror("peer might closing");
    total_clients --; // Decrement total_clients count
    printf("%d client(s) connected.\n", total_clients);
    return;
  }
  else {
    printf("message:%s\n",buffer);
  }

  const char *p = &buffer[0];
  printf("get req\n");
  http_req_t *req = request(p);
  dump_request(req);
  printf("get resp\n");
  http_resp_t *resp = response(req, "200 OK");
  dump_response(resp);
  printf("to_s\n");
  char *data = response_to_s(resp);

  // Send Message bach to the client
  send(watcher->fd, data, read, 0);
  free_request(req);
  free_response(resp);
  free(data);
  bzero(buffer, read);
  close(watcher->fd);
}

void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents){
}
