#include "Server.hpp"
// #include "../responses/Response.hpp"
#include "../requests/Request.hpp"

Server::Server() {

}

// Server::Server(std::string &hostname): _hostname(hostname) {
// }

Server::~Server() {
	for (size_t i = 0; i < getSocketsSize(); ++i) {
		close(_fds[i].fd);
	}
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

void Server::initEndpoint(HostList &hosts, short port) {
	_port = port;
	_hosts = hosts;
	_main_socketfd = socket(PF_INET, SOCK_STREAM, 0);
	if (_main_socketfd < 0) {
		throw InitialisationException("server socket endpoint is not created");
	}
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(port);
	_address_len = sizeof(_address);
	addPollfd(_main_socketfd, POLLIN);
	setSocketOpt();
	setSocketNonblock();
	bindSocketName();
}

void Server::pollfds() {
	int poll_count;

	poll_count = poll(_fds.data(), _fds.size(), 0);
	if (poll_count == -1) {
		close(_main_socketfd);
		throw PollingErrorException(strerror(errno));
	}
}

size_t Server::getSocketsSize() const {
	return _fds.size();
}

/*
*	Loop through fds to check if the event happend.
*	If event happend it revents is set accordingly.
*	From poll mannual:
*		struct pollfd {
*		int    fd;        file descriptor
*		short  events;    events to look for
*		short  revents;   events returned
*	};
*/
void Server::pollLoop() {
	for (size_t i = 0; i < getSocketsSize(); ++i) {			//loop to ckeck if revent is set
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
						throw PollingErrorException(strerror(errno));
					}
				addPollfd(client_socket, POLLIN);
				std::cout << "New connection established on fd: " << client_socket << std::endl;
			} else {										//if it is existing connection
				Request req(client_socket);
				req.parse();
				std::cout << "Request parsed:\n" << req << std::endl;
				// std::cout << "Received request:\n" << request_buffer << std::endl;
				// Response res(req);
				// const char* response = res.build();
				const char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 36\n\nResponse sent and connection closed!\n";
				send(client_socket, response, strlen(response), 0);
				std::cout << "Response sent and connection closed" << std::endl;
				close(client_socket);						//Should be closed only after xx sec according to config file;
				_fds.erase(_fds.begin() + i);
			}
		}
	}
}

void Server::setSocketOpt() {
	int opt = 1;
	if (setsockopt(_main_socketfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
		throw InitialisationException(strerror(errno));
	}
}

void Server::setSocketNonblock() {
	int flags = fcntl(_main_socketfd, F_GETFL, 0); //getting current flags
	if (flags == -1) {
		throw InitialisationException(strerror(errno));
	}
	if (fcntl(_main_socketfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		throw InitialisationException(strerror(errno));
	}
}

void Server::bindSocketName() {
	if (bind(_main_socketfd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
		throw InitialisationException(strerror(errno));
	}
}

void Server::listenPort(int backlog) {
	if (listen(_main_socketfd, backlog) < 0) {
		ListenErrorException(strerror(errno));
	}
}

const HostList &Server::getHostList() const
{
	return _hosts;
}

short Server::getPort() const
{
	return _port;
}

int Server::getMainSocketFd() const
{
	return _main_socketfd;
}

const std::vector<pollfd> &Server::getSockets() const
{
	return _fds;
}

/*Exceptions*/

Server::PollingErrorException::PollingErrorException(const char *error_msg) {
	strncpy(_error, "Pooling error: ", 15);
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::PollingErrorException::what() const throw() {
	return _error;
}

Server::InitialisationException::InitialisationException(const char *error_msg) {
	strncpy(_error, "Pooling error: ", 15);
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::InitialisationException::what() const throw() {
	return _error;
}

Server::ListenErrorException::ListenErrorException(const char *error_msg) {
	strncpy(_error, "Pooling error: ", 15);
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::ListenErrorException::what() const throw() {
	return _error;
}

std::ostream &operator<<(std::ostream &os, const Server &server) {
	os << "------------------------------" << std::endl;
	os << "Server data: " << std::endl;
	os << "Server port: " << server.getPort() << std::endl;
	os << "Server hostnames:";
	if (server.getHostList().size() == 0)
		os << "empty" << std::endl;
	for (size_t i = 0; i < server.getHostList().size(); ++i)
		os << " " << server.getHostList()[i] << std::endl;
	os << "Server sockets:";
	if (server.getSocketsSize() == 0)
		os << "empty" << std::endl;
	for (size_t i = 0; i < server.getSocketsSize(); ++i)
		os << server.getSockets()[i].fd << std::endl;
	os << "end of server data" << std::endl;
	os << "------------------------------" << std::endl;
	return os;
}
