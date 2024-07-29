#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <vector>
#include "Server.hpp"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 100

//set NON_BLOCKING SOCKET MODE
void	set_nonblocking_socket(int socket_fd)
{
	int flags = fcntl(socket_fd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl F_GETFL");
		exit(EXIT_FAILURE);
	}
	if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl F_SETFL");
		exit(EXIT_FAILURE);
	}
}

//Set socket options to address binding error and load balance enhancement
void	set_options_socket(int socket_fd)
{
	int opt = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
		close(socket_fd);
		exit(EXIT_FAILURE);
	}
}

//Bind the socket to the network address and port
void	set_bind_name_socket(int socket_fd, sockaddr_in address)
{
	if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		close(socket_fd);
		exit(EXIT_FAILURE);
	}
}

void request_handling(int socket_fd, sockaddr_in address)
{
	Server server;

	server.addPollfd(socket_fd, POLLIN); //adds socket to the list
	server.initMainSocket(socket_fd, address); //sets socket_fd and server main(listening) socket

	while (true) {
		server.pollfds(); // sets revents status to all sockets in list
		server.pollLoop(); // loops through sockets and handles if new request recieved on socket
	}
}

int	main() {
	int socket_fd;
	sockaddr_in address;

	// Creating socket file descriptor
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	set_options_socket(socket_fd);
	set_nonblocking_socket(socket_fd);
	set_bind_name_socket(socket_fd, address);

	if (listen(socket_fd, 3) < 0) {
		perror("listen");
		close(socket_fd);
		exit(EXIT_FAILURE);
	}
	std::cout << "Server is listening on port " << PORT << std::endl;
	request_handling(socket_fd, address);
	return 0;
}
