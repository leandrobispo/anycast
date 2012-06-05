#include "common.h"

#include <arpa/inet.h>

#include <string.h>

#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>

bool
setnonblocking(int fd)
{
  int opts;
  if ((opts = fcntl(fd, F_GETFL)) == -1)
    return false;

  opts = opts | O_NONBLOCK;
  return (fcntl(fd, F_SETFL, opts) != -1);
}

int
init_socket(int efd, uint16_t sock_type, struct sockaddr *srv, uint16_t srvlen, size_t simultaneous_connection)
{
  struct epoll_event ev;
  static int optval = 1;
  int fd;

  if ((fd = socket(sock_type, SOCK_STREAM,  0)) == -1)
    return -1;

  if (!setnonblocking(fd))
    goto error;

  memset(&ev, 0, sizeof(struct epoll_event));

  ev.data.fd = fd;
  ev.events  = EPOLLIN | EPOLLET;

  if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1)
    goto error;

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  int on = 1;
  if (srv->sa_family == AF_INET6)
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));

  if (bind(fd, srv, srvlen) == -1)
    goto error;

  if (listen(fd, simultaneous_connection) == -1)
    goto error;

  return fd;
error:
  close(fd);
  return -1;
}


