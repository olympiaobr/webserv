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
#define BACKLOG 3

void request_handling(Server server)
{
	while (true) {
		server.pollfds(); // sets revents status to all sockets in list
		server.pollLoop(); // loops through sockets and handles if new request recieved on socket
	}
}

int	main() {
	Server server;

	server.initEndpoint(PORT);
	server.listenPort(BACKLOG);
	std::cout << "Server is listening on port " << PORT << std::endl;
	request_handling(server);
	return 0;
}
