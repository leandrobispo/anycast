#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <malloc.h>

#include <getopt.h>
#include <string.h>

static int
start_proxy(uint16_t port, uint16_t num_of_nodes)
{
  for (;;) {
  
    //TODO: IMPLEMENT ME!!
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
    "\t-p --port=<port>\n",
    app_name
  );
}

int
main(int argc, char **argv)
{
  uint16_t port   = 0;
  uint16_t number = NULL;

  struct option opts[] = {
    { "port"       , 1, 0, 'p' },
    { "node-number", 1, 0, 'n' },
    { NULL         , 0, 0, 0   }
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "p:n:", opts, NULL)) != -1) {
    switch(opt) {
      case 'p':
        port = atoi(optarg);
      break;
      case 'n':
        number = atoi(optarg);
      break;
      default:
        print_usage(argv[0]);
        return 1;
    }
  }

  if (!port || !number) {
    print_usage(argv[0]);
    return 1;
  }

  int ret = start_proxy(port, number);

  return ret;
}
