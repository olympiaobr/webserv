#include "Server.hpp"

Server::Server() {
}

Server::~Server() {
	for (size_t i = 0; i < getSocketsSize(); ++i) {
		close(_fds[i].fd);
	}
}

void Server::_addNewClient(int client_socket)
{
	int new_client_socket = accept(_main_socketfd, (struct sockaddr *)&_address, (socklen_t*)&_address_len);
	if (new_client_socket < 0)
		return ;
	fcntl(client_socket, F_SETFL, O_NONBLOCK);
	addPollfd(new_client_socket, POLLIN | POLLOUT);
	Session new_ses(new_client_socket);
	_sessions[new_client_socket] = new_ses;
	std::cout << "new client added to " << new_client_socket << std::endl;
}

void Server::_serveExistingClient(Session &client, size_t i)
{
	// Request&	req = client.request;
	Response&	res = client.response;
	int			client_socket = client.client_id;

	try {
		client.recieveData();
		if (client.status == client.S_NEW || client.status == client.S_DONE)
			client.newRequest(_configs);
		else if (client.status == client.S_REQUEST)
			std::cout << "recieved more data" << std::endl;
		// else
		// 	throw Request::ParsingErrorException(Request::BAD_REQUEST, "unexpected error");
	} catch (Request::SocketCloseException &e) {
		_cleanChunkFiles(client_socket);
		close(client_socket);
		_fds.erase(_fds.begin() + i);
		_sessions.erase(client_socket);
		std::cout << "socket closed on " << client_socket << std::endl;
		return ;
	} catch (Request::ParsingErrorException& e) {
		if (e.type == Request::BAD_REQUEST)
			res.setStatus(400);
		else if (e.type == Request::CONTENT_LENGTH)
			res.setStatus(413);
		else if (e.type == Request::FILE_SYSTEM)
			res.setStatus(500);
		_cleanChunkFiles(client_socket);
	}
	/*******************/
	int response_code = res.getStatusCode();
	if (response_code <= -1)
		res.setStatus(500);
	if (response_code >= 500 || response_code >= 400) {
		_cleanChunkFiles(client_socket);
	}
	std::cout << "new request recieved on " << client_socket << std::endl;
}

void Server::_processRequest(Session &client, size_t i)
{
	(void)i;

	Request& req = client.request;
	Response& res = client.response;

	if (req.isTargetingCGI())
	{
		try
		{
			std::string scriptPath = req.getScriptPath();
			if (!utils::fileExists(scriptPath))
			{
				res.setStatus(404);
			}
			else
			{
				CGIHandler cgiHandler(scriptPath, req);
				std::string cgiOutput = cgiHandler.execute();
				if (cgiOutput.empty())
				{
					res.setStatus(500);
				}
				else
				{
					std::cout << "start" << std::endl;
					res.setStatus(200);
					res.addHeader("Content-Type", "text/html");
					res.addHeader("Set-Cookie", req.getSession());
					res.generateCGIResponse(cgiOutput);
					std::cout << "end" << std::endl;
				}
			}
		}
		catch (std::runtime_error &e)
		{
			std::cerr << e.what() << std::endl;
			res.setStatus(500);
		}
	}
	else
	{
		/* Configure response */
		res.initialize(req);
		/*********************/

		/* Parse request body */
		if (res.getStatusCode() < 300 && req.getMethod() == "POST")
			req.parseBody(*this);
		/*********************/
	}
	client.status = client.S_RESPONSE;
	std::cout << "response processed on " << client.client_id << std::endl;
}

void Server::_sendResponse(Session &client, size_t i)
{
	(void)i;

	client.sendResponse();
	std::cout << "response sent on " << client.client_id << std::endl;
	if (client.response.getContentLength() == 0)
		client.status = client.S_DONE;
	// 	_cleanChunkFiles(_fds[i].fd);
	// 	close(_fds[i].fd);
	// 	_fds.erase(_fds.begin() + i);
	// 	_sessions.erase(_fds[i].fd);
	// }
}

    void Server::addPollfd(int socket_fd, short events)
{
    pollfd fd;
	fd.fd = socket_fd;
	fd.events = events;
	fd.revents = 0;
	_push(fd);
}

void Server::_push(pollfd client_pollfd) {
	_fds.push_back(client_pollfd);
}

void Server::initEndpoint(short port, const std::vector<ServerConfig> &configs)
{
	_port = port;
	_configs = configs;

	_main_socketfd = socket(PF_INET, SOCK_STREAM, 0);
	if (_main_socketfd < 0) {
		throw InitialisationException("server socket endpoint is not created");
	}
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(port);
	_address_len = sizeof(_address);
	addPollfd(_main_socketfd, POLLIN);
	_setSocketOpt();
	_setSocketNonblock();
	_bindSocketName();
}

void Server::pollfds() {
	int poll_count;

	poll_count = poll(_fds.data(), _fds.size(), 0);
	// for (size_t i = 0; i < _fds.size(); ++i)
	// {
	// 	if (_fds[i].fd != _main_socketfd && _checkRequestTimeout(_fds[i].fd)) {
	// 		std::cerr << "Request timeout" << std::endl;
	// 		close(_fds[i].fd);
	// 		_fds.erase(_fds.begin() + i);
	// 	}
	// }
	if (poll_count == -1) {
		close(_main_socketfd);
		_fds.erase(_fds.begin());
		throw PollingErrorException(strerror(errno));
	}
}

size_t Server::getSocketsSize() const {
	return _fds.size();
}

void Server::pollLoop() {
	for (size_t i = 0; i < getSocketsSize(); ++i) {
		if (_fds[i].revents & POLLERR) {
			std::cerr << "Polling error: " << errno << std::endl;
			_cleanChunkFiles(_fds[i].fd);
			close(_fds[i].fd);
			_fds.erase(_fds.begin() + i);
			_sessions.erase(_fds[i].fd);
			if (_fds[i].fd == _main_socketfd)
				throw PollingErrorException("error from poll() function");
		}
		int client_socket = _fds[i].fd;
		if (_fds[i].revents & POLLIN) {
			if (client_socket == _main_socketfd) {
				_addNewClient(client_socket);
			} else {
				_serveExistingClient(_sessions[client_socket], i);
			}
		} else if (_fds[i].revents & POLLOUT) {
			Session &client = _sessions[client_socket];
			if (client.status != Session::S_NEW && client.status != Session::S_DONE)
				_processRequest(client, i);
			if (client.status == Session::S_RESPONSE)
				_sendResponse(client, i);
		}
	}
}

void Server::_setSocketOpt() {
	int opt = 1;
	if (setsockopt(_main_socketfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
		throw InitialisationException(strerror(errno));
	}
}

void Server::_setSocketNonblock() {
	int flags = fcntl(_main_socketfd, F_GETFL, 0); //getting current flags
	if (flags == -1) {
		throw InitialisationException(strerror(errno));
	}
	if (fcntl(_main_socketfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		throw InitialisationException(strerror(errno));
	}
}

void Server::_bindSocketName() {
	if (bind(_main_socketfd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
		throw InitialisationException(strerror(errno));
	}
}

// void Server::_setRequestTime(int client_socket)
// {
// 	_request_time[client_socket] = utils::getCurrentTime();
// }

// bool Server::_checkRequestTimeout(int client_socket)
// {
// 	if (difftime(utils::getCurrentTime(), _request_time[client_socket]) >= REQUEST_TIMEOUT)
// 		return true;
// 	return false;
// }

void Server::_cleanChunkFiles(int client_socket)
{
	if (utils::checkChunkFileExistance(utils::buildPath(client_socket, TEMP_FILES_DIRECTORY)))
		utils::deleteFile(utils::buildPath(client_socket, TEMP_FILES_DIRECTORY));
}

void Server::listenPort(int backlog) {
	if (listen(_main_socketfd, backlog) < 0) {
		throw ListenErrorException(strerror(errno));
	}
}

// const HostList &Server::getHostList() const {
// 	return _hosts;
// }

short Server::getPort() const {
	return _port;
}

int Server::getMainSocketFd() const {
	return _main_socketfd;
}

const std::vector<pollfd> &Server::getSockets() const {
	return _fds;
}

void Server::RUN(std::vector<Server> servers) {
	for (size_t i = 0; i < servers.size(); ++i) {
		servers[i].listenPort(BACKLOG);
		std::cout << BLUE << "Server " << i + 1 << " is listening on port "
			<< servers[i].getPort() << RESET << std::endl;
	}
	while (true)
	{
		for (size_t i = 0; i < servers.size(); ++i) {
			try {
				servers[i].pollfds();
				servers[i].pollLoop();
			} catch (PollingErrorException& e) {
				std::cout << RED << e.what() << RESET << std::endl;
				std::cout << RED << "Server " << i + 1 << " on port "
					<< servers[i].getPort() << " stoped" << RESET << std::endl;
				servers.erase(servers.begin() + i);
			}
		}
	}
}

/*Exceptions*/

Server::PollingErrorException::PollingErrorException(const char *error_msg) {
	strncpy(_error, "Pooling error: ", 16);
	_error[sizeof(_error) - 1] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::PollingErrorException::what() const throw() {
	return _error;
}

Server::InitialisationException::InitialisationException(const char *error_msg) {
	std::memset(_error, 0, 256);
	strncpy(_error, "Initialisation error: ", 23);
	_error[sizeof(_error) - 1] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::InitialisationException::what() const throw() {
	return _error;
}

Server::ListenErrorException::ListenErrorException(const char *error_msg) {
	std::memset(_error, 0, 256);
	strncpy(_error, "Listening error: ", 18);
	_error[sizeof(_error) - 1] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::ListenErrorException::what() const throw() {
	return _error;
}

Server::RuntimeErrorException::RuntimeErrorException(const char *error_msg) {
	std::memset(_error, 0, 256);
	strncpy(_error, "Runtime error: ", 16);
	_error[sizeof(_error) - 1] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::RuntimeErrorException::what() const throw() {
	return _error;
}

std::ostream &operator<<(std::ostream &os, const Server &server) {
	os << "------------------------------" << std::endl;
	os << "Server data: " << std::endl;
	os << "Server port: " << server.getPort() << std::endl;
	os << "Server hostnames:";
	// if (server.getHostList().size() == 0)
	// 	os << "empty" << std::endl;
	// for (size_t i = 0; i < server.getHostList().size(); ++i)
	// 	os << " " << server.getHostList()[i] << std::endl;
	os << "Server sockets:";
	if (server.getSocketsSize() == 0)
		os << "empty" << std::endl;
	for (size_t i = 0; i < server.getSocketsSize(); ++i)
		os << server.getSockets()[i].fd << std::endl;
	os << "end of server data" << std::endl;
	os << "------------------------------" << std::endl;
	return os;
}

// Outstream::Outstream(ssize_t bytes, const char *buffer, int status_code) {
// 	counter = 0;
// 	bytes_to_send = bytes;
//     status = status_code;
// 	this->buffer = new char[bytes_to_send];
// 	std::memset(this->buffer, 0, bytes_to_send);
// 	memmove(this->buffer, buffer, bytes_to_send);
// }

// Outstream::Outstream(const Outstream &src)
// {
// 	if (src.bytes_to_send > 0) {
// 		buffer = new char[src.bytes_to_send];
// 		std::memmove(buffer, src.buffer, src.bytes_to_send);
// 	}
// 	else
// 		buffer = 0;
// 	bytes_to_send = src.bytes_to_send;
// 	counter = src.counter;
//     status = src.status;
// }

// Outstream &Outstream::operator=(const Outstream &src)
// {
// 	if (this != &src) {
// 		if (buffer)
// 			delete[] buffer;
// 		buffer = new char[src.bytes_to_send];
// 		std::memmove(buffer, src.buffer, src.bytes_to_send);
// 		bytes_to_send = src.bytes_to_send;
// 		counter = src.counter;
//         status = src.status;
// 	}
// 	return *this;
// }
