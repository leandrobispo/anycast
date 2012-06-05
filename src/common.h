#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>

#include <stdint.h>
#include <stdbool.h>

bool
setnonblocking(int fd);

int
init_socket(int efd, uint16_t sock_type, struct sockaddr *srv, uint16_t srvlen, size_t simultaneous_connection);

#endif
