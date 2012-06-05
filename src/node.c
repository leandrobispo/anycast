#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <malloc.h>

#include <getopt.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <netdb.h>

static int
start_node(const char * const proxy_address, uint16_t port)
{
  int fd;
  struct in6_addr srv; 
  struct addrinfo hints, *res = NULL;

  memset(&hints, 0x00, sizeof(hints));
  hints.ai_flags    = AI_NUMERICSERV;
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (inet_pton(AF_INET, proxy_address, &srv) == 1) {
    hints.ai_family = AF_INET;
    hints.ai_flags |= AI_NUMERICHOST;
  }
  else if (inet_pton(AF_INET6, proxy_address, &srv) == 1) {
    hints.ai_family = AF_INET6;
    hints.ai_flags |= AI_NUMERICHOST;
  }

  char srv_port[5];
  sprintf(srv_port, "%u", port);

  if (getaddrinfo(proxy_address, srv_port, &hints, &res) != 0) {
    //TODO: Handle the error!
    return 1;
  }

  if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
    //TODO: Handle the error!
    return 1;
  }

  if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
    //TODO: Handle the error!
    return 1;
  }

  //TODO: WHILE!!!

  if (res)
    freeaddrinfo(res); 
}

static void
print_usage(const char * const app_name)
{
  fprintf(stderr, "Usage: %s [options]\n"
    "Calculator Node Implementation\n"
    "Options:\n"
    "\t-P --proxy=<proxy>\n"
    "\t-p --proxy-port=<port>\n",
    app_name
  );
}

int
main(int argc, char **argv)
{
  uint16_t port   = 0;
  char     *proxy = NULL;

  struct option opts[] = {
    { "port"      , 1, 0, 'p' },
    { "proxy-port", 1, 0, 'P' },
    { NULL        , 0, 0, 0   }
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "p:P:", opts, NULL)) != -1) {
    switch(opt) {
      case 'p':
        port = atoi(optarg);
      break;
      case 'P':
        proxy = strdup(optarg);
      break;
      default:
        print_usage(argv[0]);
        return 1;
    }
  }

  if (!port || !proxy) {
    print_usage(argv[0]);
    return 1;
  }

  int ret = start_node(proxy, port);
 
  free(proxy); 

  return ret;
}
