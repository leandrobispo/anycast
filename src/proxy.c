#include <stdio.h>
#include "list.h"
#include <stdint.h>
#include <stdbool.h>

#include <malloc.h>

#include <getopt.h>
#include <string.h>

#include <arpa/inet.h>

#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>


#define MAX_SIMULTANEOUS_CONNECTION 300

#define CLIENT 0
#define NODE   1

#define SCHED_RANDOM 1
#define SCHED_RR     2

struct connection_context {
  int fd;
  int type;

  struct list_head client_queue;
};

struct dispatcher {
  int pos;
  int size;
  struct connection_context **queue;
};

static void
call_client(struct connection_context *dctx, const char *buffer, ssize_t bufflen)
{
  struct connection_context *ctx, *tmp;
  list_for_each_entry_safe(ctx, tmp, &dctx->client_queue, client_queue) {
    send(ctx->fd, buffer, bufflen, 0); 
    list_del_init(&ctx->client_queue);
    break;
  }  
}

static void
call_dispatcher(int scheduler_method, struct connection_context *ctx, struct dispatcher *d, const char *buffer, ssize_t bufflen)
{
  srand(time(0));

  if (!d->size)
    send(ctx->fd, "Not possible to process", strlen("Not possible to process"), 0);
  else if (scheduler_method == SCHED_RR) {
    struct connection_context *dctx = d->queue[d->pos];
    ++d->pos;
    if (d->pos == d->size) d->pos = 0;
    send(dctx->fd, buffer, bufflen, 0);
    list_add_tail(&ctx->client_queue, &dctx->client_queue);
  }
  else {
    int pos = (rand() % d->size);
    struct connection_context *dctx = d->queue[pos];
    send(dctx->fd, buffer, bufflen, 0);
    list_add_tail(&ctx->client_queue, &dctx->client_queue);
  }
}

static struct connection_context *
register_client(int fd)
{
  struct connection_context *ctx = malloc(sizeof(struct connection_context));

  ctx->fd   = fd;
  ctx->type = CLIENT;

  INIT_LIST_HEAD(&ctx->client_queue);
  return ctx;
}

static struct connection_context *
register_node(int fd, struct dispatcher *d) {
  struct connection_context *ctx = malloc(sizeof(struct connection_context));

  ctx->fd   = fd;
  ctx->type = NODE;

  ++(d->size);

  d->queue = realloc(d->queue, sizeof(struct connection_context *) * d->size);
  d->queue[d->size - 1] = ctx;

  INIT_LIST_HEAD(&ctx->client_queue);

  return ctx;
}

static void
unregister_node(int fd, struct dispatcher *d) {
  struct connection_context **curr = d->queue;

  int size = d->size;
  while ((*curr)->fd != fd) {
    --size;
    ++curr;
  }

  while (size - 1) {
    *curr = *(curr + 1);
    ++curr;
    --size;
  }


  --d->size;
  if (d->pos == d->size)
    d->pos = 0;
}

static int
start_proxy(uint16_t port_client, uint16_t port_node, uint16_t num_of_nodes, uint8_t scheduler_method)
{
  int efd, tcp4fd, tcp6fd, tcp_node4fd, tcp_node6fd;

  struct sockaddr_in  srv4, srv_node4, cli4;
  struct sockaddr_in6 srv6, srv_node6, cli6;

  socklen_t client_len4 = sizeof(srv4);
  socklen_t client_len6 = sizeof(srv6);

  uint16_t max_conn = (MAX_SIMULTANEOUS_CONNECTION * 2) + (num_of_nodes * 2);

#ifdef EPOLLRDHUP
  if ((efd = epoll_create1(0)) == -1) {
#else
  if ((efd = epoll_create(max_conn + 5)) == -1) {
#endif
    fprintf(stderr, "Problems to start an epoll file descriptor\n");
    return 1;
  }

  struct dispatcher d;
  memset(&d, 0, sizeof(d));

  memset(&srv4     , 0, sizeof(srv4));
  memset(&srv_node4, 0, sizeof(srv_node4));
  memset(&srv6     , 0, sizeof(srv6));
  memset(&srv_node6, 0, sizeof(srv_node6));

  srv4.sin_family      = AF_INET;
  srv4.sin_port        = htons(port_client);
  srv4.sin_addr.s_addr = INADDR_ANY;

  srv6.sin6_family = AF_INET6;
  srv6.sin6_port   = htons(port_client);
  srv6.sin6_addr   = in6addr_any;

  srv_node4.sin_family      = AF_INET;
  srv_node4.sin_port        = htons(port_node);
  srv_node4.sin_addr.s_addr = INADDR_ANY;

  srv_node6.sin6_family = AF_INET6;
  srv_node6.sin6_port   = htons(port_node);
  srv_node6.sin6_addr   = in6addr_any;

  struct epoll_event tcpev, events[max_conn + 5];
  if ((tcp4fd = init_socket(efd, AF_INET, (struct sockaddr *) &srv4, client_len4, MAX_SIMULTANEOUS_CONNECTION)) == -1) {
    fprintf(stderr, "Cannot initialize the IN4\n");
    //TODO: HANDLE THE ERROR
    return 1;
  }

  if ((tcp_node4fd = init_socket(efd, AF_INET, (struct sockaddr *) &srv_node4, client_len4, num_of_nodes)) == -1) {
    fprintf(stderr, "Cannot initialize the OUT4\n");
    //TODO: HANDLE THE ERROR
    return 1;
  }

  if ((tcp6fd = init_socket(efd, AF_INET6, (struct sockaddr *) &srv6, client_len6, MAX_SIMULTANEOUS_CONNECTION)) == -1) {
    fprintf(stderr, "Cannot initialize the OUT6\n");
    //TODO: HANDLE THE ERROR
    return 1;
  }

  if ((tcp_node6fd = init_socket(efd, AF_INET6, (struct sockaddr *) &srv_node6, client_len6, num_of_nodes)) == -1) {
    fprintf(stderr, "Cannot initialize the IN6\n");
    //TODO: HANDLE THE ERROR
    return 1;
  }

  int nfds;
  for (;;) {
    if ((nfds = epoll_wait(efd, events, max_conn + 5, -1)) == -1) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      else {
        //TODO: HANDLE THE ERROR
        return 1;
      }
    }

    int i;
    for (i = 0; i < nfds; ++i) {
      if ((events[i].data.fd == tcp4fd || events[i].data.fd == tcp_node4fd) && events[i].events) {
        int new_fd;
        while ((new_fd = accept(events[i].data.fd, (struct sockaddr *) &cli4, &client_len4)) != -1) {
          setnonblocking(new_fd);
          if (events[i].data.fd == tcp4fd)
            tcpev.data.ptr = (void *) register_client(new_fd);
          else
            tcpev.data.ptr = (void *) register_node(new_fd, &d);

#ifdef EPOLLRDHUP
          tcpev.events  = EPOLLIN | EPOLLET | EPOLLRDHUP;
#else
          tcpev.events  = EPOLLIN | EPOLLET | EPOLLHUP;
#endif

          if (epoll_ctl(efd, EPOLL_CTL_ADD, new_fd, &tcpev) == -1) {
            fprintf(stderr, "Problems to add the new TCP connection to the poll: %s\n", strerror(errno));
            close(new_fd);
            continue;
          }
        }
      }
      else if ((events[i].data.fd == tcp6fd || events[i].data.fd == tcp_node6fd) && events[i].events) {
        int new_fd;
        while ((new_fd = accept(events[i].data.fd, (struct sockaddr *) &cli6, &client_len6)) != -1) {
          setnonblocking(new_fd);
          if (events[i].data.fd == tcp6fd)
            tcpev.data.ptr = (void *) register_client(new_fd);
          else
            tcpev.data.ptr = (void *) register_node(new_fd, &d);

#ifdef EPOLLRDHUP
          tcpev.events  = EPOLLIN | EPOLLET | EPOLLRDHUP;
#else
          tcpev.events  = EPOLLIN | EPOLLET | EPOLLHUP;
#endif

          if (epoll_ctl(efd, EPOLL_CTL_ADD, new_fd, &tcpev) == -1) {
            fprintf(stderr, "Problems to add the new TCP connection to the poll: %s\n", strerror(errno));
            close(new_fd);
            continue;
          }
        }
      }
      else
      {
        struct connection_context *ctx = (struct connection_context *) events[i].data.ptr;
#ifdef EPOLLRDHUP
        if (events[i].events & EPOLLRDHUP) {
#else
        if (events[i].events & EPOLLHUP) {
#endif
          epoll_ctl(efd, EPOLL_CTL_DEL, ctx->fd, NULL);
          close(ctx->fd);
          if (ctx->type == NODE)
            unregister_node(ctx->fd, &d);

          list_del_init(&ctx->client_queue);
          free(ctx);
          continue;
        }

        char buffer[8000];
        ssize_t bytes_recv = recv(ctx->fd, buffer, 8000, 0);

        if (ctx->type == CLIENT)
          call_dispatcher(scheduler_method, ctx, &d, buffer, bytes_recv);
        else
          call_client(ctx, buffer, bytes_recv);
      }
    }
  }

  return 0;
}

static void
print_usage(const char * const app_name)
{
  fprintf(stderr, "Usage: %s [options]\n"
    "Calculator proxy Implementation\n"
    "Options:\n"
    "\t-n --node-number=<number>\n"
    "\t-c --port-client=<port>\n"
    "\t-s --port-node=<port>\n"
    "\t-R --sched-rr\n"
    "\t-r --sched-random\n",
    app_name
  );
}

int
main(int argc, char **argv)
{
  uint8_t  type        = 0;
  uint16_t port_client = 0;
  uint16_t port_node   = 0;
  uint16_t number      = 0;

  struct option opts[] = {
    { "port-client" , 1, 0, 'c' },
    { "port-node"   , 1, 0, 's' },
    { "node-number" , 1, 0, 'n' },
    { "sched-rr"    , 1, 0, 'R' },
    { "sched-random", 1, 0, 'r' },
    { NULL          , 0, 0, 0   }
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "s:c:n:rR", opts, NULL)) != -1) {
    switch(opt) {
      case 'c':
        port_client = atoi(optarg);
      break;
      case 's':
        port_node = atoi(optarg);
      break;
      case 'n':
        number = atoi(optarg);
      break;
      case 'r':
        type |= SCHED_RANDOM;
      break;
      case 'R':
        type |= SCHED_RR;
      break;
      default:
        print_usage(argv[0]);
        return 1;
    }
  }

  if (!port_client || !port_node || !number || type == 0 || type > 2) {
    print_usage(argv[0]);
    return 1;
  }

  return start_proxy(port_client, port_node, number, type);
}
