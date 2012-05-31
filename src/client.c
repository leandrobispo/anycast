#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <dirent.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <linux/if_ether.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/resource.h>

#include <netinet/in.h>
#include <netdb.h>

#include <string.h>

#define MAX_EVENTS 2
#define BUFF_MAX BUFSIZ + 1

static bool
setnonblocking(int fd)
{
  int opts;
  if ((opts = fcntl(fd, F_GETFL)) == -1)
    return false;

  opts = opts | O_NONBLOCK;
  if (fcntl(fd, F_SETFL, opts) == -1)
    return false;

  return true;
}


static void
send_request(int sock_fd, char * buffer)
{
	ssize_t recv_bytes;
	char recv_buffer[BUFF_MAX];

	if (send(sock_fd, buffer, strlen(buffer), 0) != strlen(buffer)) {
		printf("Could not send message.\n");
	}

	send(sock_fd, (char *) 13, 1, 0);

	recv_bytes = recv(sock_fd, recv_buffer, BUFF_MAX, 0);
	recv_buffer[recv_bytes] = 0;

	printf("%s\n> ", recv_buffer);	
	fflush(stdout);

}

static void
process_command (int sock_fd, char * buffer)
{
	
	if (strncasecmp("help", buffer, strlen("help")) == 0) {
	    printf("\nManual da Calculadora\nOprerações: + , - , x , /\nUso: operação <operando 1> <operando 2>\nPara sair: quit\n\n> ");
		return;
	} 
	else if (strncasecmp("quit", buffer, strlen("quit")) == 0) {
		close(sock_fd);
		exit(0);
	} 
	else {
		send_request(sock_fd, buffer);
	}

}	

int 
main (int argc, char **argv)
{
	struct sockaddr_in srv;
	struct hostent *registerDNS;
	char buffer[BUFF_MAX];
	char *hostName;
	char *data;

	int efd, sock_fd, nfds;
	struct epoll_event in_event, events[MAX_EVENTS];

	if (argc != 3) {
		printf("Usage: client <host> <port>\n");
		exit(1);
	}

	hostName = argv[1];

	if ((registerDNS = gethostbyname(hostName)) == NULL) {
		printf("Could not get IP address. May not continue.\n");
		exit(1);
	}

	bcopy((char *) registerDNS->h_addr, (char *) &srv.sin_addr, registerDNS->h_length);

	srv.sin_port = htons(atoi(argv[2]));
	srv.sin_family = AF_INET;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not initialize SOCKET.\n");
		exit(1);
	}

	if (!setnonblocking(STDIN_FILENO)) {
		printf("Problems to initialize STDIN.\n");
		exit(1);
	}

	if ((connect(sock_fd, (struct sockaddr *) &srv, sizeof(srv)) < 0)) {
		printf("Could not connect to the server.\n");
		exit(1);
	}	

	memset(&in_event, 0, sizeof(struct epoll_event));

	in_event.data.fd = STDIN_FILENO;
	in_event.events = EPOLLIN | EPOLLET;

#ifdef EPOLLRDHUP
	if ((efd = epoll_create1(0)) == -1) {
#else
	if ((efd = epoll_create(MAX_EVENTS)) == -1) {
#endif
		printf("Could not initialize EPOLL.\n");
		exit(1);
	}

	if (epoll_ctl(efd, EPOLL_CTL_ADD, STDIN_FILENO, &in_event) == -1) {
		printf("Could not initialize EPOLL.\n");
		exit(1);
	}

	printf("> ");
	fflush(stdout);

	for (;;) {
		if ((nfds = epoll_wait(efd, events, MAX_EVENTS, 200)) == -1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			else {
				printf("Internal error. Application may not continue.\n");
				exit(1);
			}

		}

		int i;
		for (i = 0; i < nfds; i++) {
			if (events[i].data.fd == STDIN_FILENO && events[i].events) {
				int j = 0;
				char c;
				while (read(STDIN_FILENO, &c, 1) != -1) {
					if (j < BUFF_MAX -1 && c != '\n') {
						buffer[j++] = c;
					}
				}

				fflush(stdin);
				buffer[j] = 0;

				process_command (sock_fd, buffer);
			}
		}
	}
}	
