#include "Server.hpp"

Server::Server() {
}

Server::~Server() {
}

void Server::addPollfd(int socket_fd, short events) {
	pollfd fd;
	fd.fd = socket_fd;
	fd.events = events;
	_push(fd);
}

void Server::_push(pollfd client_pollfd) {
	_fds.push_back(client_pollfd);
}

void Server::initMainSocket(int main_socketfd, sockaddr_in address) {
	_main_socketfd = main_socketfd;
	_address = address;
	_address_len = sizeof(address);
}

void Server::pollfds() {
	int poll_count;

	poll_count = poll(_fds.data(), _fds.size(), -1);
	if (poll_count == -1) {
		close(_main_socketfd);
		throw PollingErrorException("error from poll() function");
	}
}

size_t Server::socketsSize() {
	return _fds.size();
}

Server::PollingErrorException::PollingErrorException(const char *error_place)
{
	strncpy(_error, "Pooling error: ", 15);
	strlcat(_error, error_place, 256);
}

const char *Server::PollingErrorException::what() const throw()
{
	return _error;
}

/*	Loop through fds to check if the event happend.
*	If event happend it revents is set accordingly.
*	From poll mannual:
*		struct pollfd {
*		int    fd;        file descriptor
*		short  events;    events to look for
*		short  revents;   events returned
*	};
*/
void Server::pollLoop() {
	for (size_t i = 0; i < socketsSize(); ++i) {			//loop to ckeck if revent is set
		if (_fds[i].revents & POLLERR) {					//man poll
			close(_main_socketfd);
			throw PollingErrorException("error from poll() function");
		}
		if (_fds[i].revents & POLLIN) {
			int client_socket = _fds[i].fd;
			if (client_socket == _main_socketfd) {			//if it is new connection
				int client_socket = accept(_main_socketfd, (struct sockaddr *)&_address, (socklen_t*)&_address_len);
				if (client_socket < 0) {
						close(_main_socketfd);
						throw PollingErrorException("error from accept() function");
					}
				addPollfd(client_socket, POLLIN);
				std::cout << "New connection established on fd: " << client_socket << std::endl;
			} else {										//if it is existing connection
				char	request_buffer[1024];				//this must be part of Request class
				int bytes_read = read(client_socket, request_buffer, 1024);	//1024 is maximum Content-lenght, adjust accordingly
				if (bytes_read <= 0) {						// is it possible that request is 0 bytes??
					std::cerr << "Error on reading request" << std::endl;
				} else {
					std::cout << "Received request:\n" << request_buffer << std::endl;
					const char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 36\n\nResponse sent and connection closed!";
					send(client_socket, response, strlen(response), 0);
					std::cout << "Response sent and connection closed" << std::endl;
				}
				close(client_socket);						//Should be closed only after xx sec according to config file;
				_fds.erase(_fds.begin() + i);
			}
		}
	}
}
