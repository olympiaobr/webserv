#include "Session.hpp"

Session::Session()
{
}

Session::~Session()
{
}

void Session::addPollfd(int socket_fd, short events)
{
	pollfd fd;
	fd.fd = socket_fd;
	fd.events = events;
	_push(fd);
}

void Session::_push(pollfd client_pollfd)
{
	_fds.push_back(client_pollfd);
}

void Session::setMainSocketfd(int main_socketfd)
{
	_main_socketfd = main_socketfd;
}

void Session::pollfds()
{
	int poll_count;

	poll_count = poll(_fds.data(), _fds.size(), -1);
	if (poll_count == -1) {
		perror("poll");
		close(_main_socketfd);
		exit(EXIT_FAILURE);
	}
}
