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

#define PORT_1 8080
#define PORT_2 8081
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 100
#define BACKLOG 3

int	main() {
	HostList hosts;
	hosts.push_back( "www.example.com");
	hosts.push_back("api.example.com");
	hosts.push_back("www.mysite.com");
	Server server;
	Server server2;

	server.initEndpoint(hosts, PORT_1);
	server.listenPort(BACKLOG);
	server2.initEndpoint(hosts, PORT_2);
	server2.listenPort(BACKLOG);
	std::cout << "Server 1 is listening on port " << PORT_1 << std::endl;
	std::cout << "Server 2 is listening on port " << PORT_2 << std::endl;
	while (true) {
		server.pollfds(); // sets revents status to all sockets in list
		server.pollLoop(); // loops through sockets and handles if new request recieved on socket
		server2.pollfds();
		server2.pollLoop();
	}
	return 0;
}
