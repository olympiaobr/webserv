#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <vector>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 100

size_t write_data_to_file(const char *buffer, size_t buffer_size, std::string file_name);


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
	std::vector<pollfd> fds;
	int addrlen = sizeof(address);
	pollfd fd;
	char buffer[BUFFER_SIZE + 1] = {0};

	fd.fd = socket_fd;
	fd.events = POLLIN;
	fds.push_back(fd);

	while (true) {
		int poll_count = poll(fds.data(), fds.size(), 0);

		if (poll_count == -1) {
			perror("poll");
			close(socket_fd);
			exit(EXIT_FAILURE);
		}
		for (size_t i = 0; i < fds.size(); ++i)
		{
			if (fds[i].revents & POLLERR) {
				perror("poll error");
				close(socket_fd);
				exit(EXIT_FAILURE);
			}
			if (fds[i].revents & POLLIN) {
				if (fds[i].fd == socket_fd)
				{
					int new_socket;
					new_socket = accept(socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
					if (new_socket < 0) {
						perror("accept");
						close(socket_fd);
						exit(EXIT_FAILURE);
					}
					pollfd client_pollfd;
					client_pollfd.fd = new_socket;
					client_pollfd.events = POLLIN;
					fds.push_back(client_pollfd);
					std::cout << "New connection accepted" << std::endl;
				} else {
					int client_socket;
					int bytes_read;

					client_socket = fds[i].fd;
					bytes_read = recv(client_socket, buffer, BUFFER_SIZE, MSG_DONTWAIT);
					while (bytes_read > 0)
					{
						buffer[bytes_read] = 0;
						const char *body;
						if (strstr(buffer, "\r\n\r\n"))
							body = strstr(buffer, "\r\n\r\n") + 4;
						else
							body = buffer;
						write_data_to_file(body, bytes_read, "temp_name.jpg");
						bytes_read = recv(client_socket, buffer, BUFFER_SIZE, MSG_DONTWAIT);
					}
					if (bytes_read <= 0)
					{
						std::cout << "Connection closed, nothing to read" << std::endl;
						const char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 34\n\nConnection closed, nothing to read";
						send(client_socket, response, strlen(response), 0);
					} else
					{
						std::cout << "Received request:\n" << buffer << std::endl;
						const char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 36\n\nResponse sent and connection closed!";
						send(client_socket, response, strlen(response), 0);
						std::cout << "Response sent and connection closed" << std::endl;
					}
					close(client_socket);
					fds.erase(fds.begin() + i);
				}
			}
		}
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
