#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <malloc.h>

#include <getopt.h>
#include <string.h>

#define IPV4 1
#define IPV6 2

static int
register_in_proxy(const char * const server_address, uint16_t port)
{
  int fd = 0;
  return fd;
}

static int
start_node(int type, const char * const proxy_address, uint16_t port)
{
  if (type == IPV4) {
  }
  else if (type == IPV6) {
  }

  for (;;) {
  
    //TODO: IMPLEMENT ME!!
  }

  return 0;
}

static void
print_usage(const char * const app_name)
{
  fprintf(stderr, "Usage: %s [options]\n"
    "Calculator Node Implementation\n"
    "Options:\n"
    "\t-P --proxy=<proxy>\n"
    "\t-f --ivp4\n"
    "\t-s --ivp6\n"
    "\t-p --proxy-port=<port>\n",
    app_name
  );
}

int
main(int argc, char **argv)
{
  uint16_t port   = 0;
  char     *proxy = NULL;

  int type = 0;

  struct option opts[] = {
    { "port"      , 1, 0, 'p' },
    { "proxy-port", 1, 0, 'P' },
    { "ipv4"      , 1, 0, 'f' },
    { "ipv6"      , 1, 0, 's' },
    { NULL        , 0, 0, 0   }
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "p:P:fs", opts, NULL)) != -1) {
    switch(opt) {
      case 'f':
        type |= IPV4;
      break;
      case 's':
        type |= IPV6;
      break;
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

  if (!port || !proxy || type == 0 || type == 3) {
    print_usage(argv[0]);
    return 1;
  }

  int ret = start_node(type, proxy, port);
 
  free(proxy); 

  return ret;
}
