#include "Server.hpp"

Server::Server(): _buffer(0), _res_buffer(0) {
}

Server::~Server() {
	for (size_t i = 0; i < getSocketsSize(); ++i) {
		close(_fds[i].fd);
	}
	if (_buffer) {
        delete[] _buffer;
    }
    if (_res_buffer) {
        delete[] _res_buffer;
    }
}

void Server::_addNewClient(int client_socket)
{
	int new_client_socket = accept(_main_socketfd, (struct sockaddr *)&_address, (socklen_t*)&_address_len);
	if (new_client_socket < 0)
		return ;
	fcntl(client_socket, F_SETFL, O_NONBLOCK);
	addPollfd(new_client_socket, POLLIN | POLLOUT);
	_setRequestTime(new_client_socket);
}

void Server::_requestHandling(Request &req, Response &res)
{
	/* Parse request headers */
	ssize_t bytes_read = recv(req.getSocket(), _buffer, _buffer_size - 1, 0);
	if (bytes_read == 0) {
		throw Request::SocketCloseException("connection closed by client");
	} else if (bytes_read < 0) {
		throw Request::ParsingErrorException(Request::BAD_REQUEST, "malformed request");
	}
	req.setBufferLen(bytes_read);
	if (_streams.find(req.getSocket()) != _streams.end()) {
		_processStream(_streams.find(req.getSocket())->second);
		res.setStatus(200);
		return ;
	}
    if (_res_streams.find(req.getSocket()) != _res_streams.end() && _res_streams[req.getSocket()].status >= 400) {
        return ;
    }
	req.parseHeaders(_sessions);
	/*************************/
	if (req.isTargetingCGI()) {
		try {
			std::string scriptPath = req.getScriptPath();
			std::cout << "Executing CGI for path: " << scriptPath << std::endl;  // Debugging output

			if (!utils::fileExists(scriptPath)) {
				res = Response(_config, 404, _res_buffer, _res_buffer_size);
			} else {
				CGIHandler cgiHandler(scriptPath, req, _config);
				std::string cgiOutput = cgiHandler.execute();
				if (cgiOutput.empty()) {
					res = Response(_config, 500, _res_buffer, _res_buffer_size);
				} else {
					res.setStatus(200);
					res.addHeader("Content-Type", "text/html");
					res.addHeader("Set-Cookie", req.getSession());
					res.generateCGIResponse(cgiOutput);
				}
			}
		} catch (std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			res = Response(_config, 500, _res_buffer, _res_buffer_size);
		}
	} else {
		/* Configure response */
		res = Response(req, _config, _res_buffer, _res_buffer_size);
		/*********************/
		/* Parse request body */
        // try {
            if (res.getStatusCode() < 300 && req.getMethod() == "POST")
                req.parseBody(*this);
        // } catch (Request::ParsingErrorException& e) {
        //     std::cerr << e.what() << std::endl;
        //     res = Response(_config, 413, _res_buffer, _res_buffer_size);

        // }
		/*********************/
	}

}

void Server::_serveExistingClient(int client_socket, size_t i)
{
	Request req(client_socket, _config, _buffer, _buffer_size);
	Response res(req, _config, _res_buffer, _res_buffer_size);
	/* Request handling */
	try {
		_requestHandling(req, res);
	} catch (Request::SocketCloseException &e) {
		std::cout << e.what() << std::endl;
		_cleanChunkFiles(client_socket);
		close(client_socket);
		_fds.erase(_fds.begin() + i);
		return ;
	} catch (Request::ParsingErrorException& e) {
		if (e.type == Request::BAD_REQUEST)
			res = Response(_config, 400, _res_buffer, _res_buffer_size);
		else if (e.type == Request::CONTENT_LENGTH)
			res = Response(_config, 413, _res_buffer, _res_buffer_size);
		else if (e.type == Request::FILE_SYSTEM)
			res = Response(_config, 500, _res_buffer, _res_buffer_size);
		_cleanChunkFiles(client_socket);
	}
	/*******************/
	if (res.getStatusCode() == -1)
		res = Response(_config, 500, _res_buffer, _res_buffer_size);

	Outstream outsteam(res.getContentLength(), res.getContent(), res.getStatusCode());
    if (_res_streams.find(client_socket) == _res_streams.end()) {
        _res_streams[client_socket] = outsteam;
    }
	else if (res.getStatusCode() != 200 && _res_streams[client_socket].status == 200) {
		_res_streams[client_socket] = outsteam;
	}
	int response_code = res.getStatusCode();
	if (response_code >= 500 || response_code >= 400) {
		_cleanChunkFiles(client_socket);
        // close(client_socket);
        // _fds.erase(_fds.begin() + i);
	}
}

void Server::_processStream(Stream stream)
{
	int file_fd = stream.file_fd;
	int client_socket = stream.req.getSocket();
	char* buffer = _buffer;

	std::string& boundary = stream.boundary;
	std::string boundary_end = boundary + "--";
	char *boundary_pos = utils::strstr(buffer, boundary.c_str(), _buffer_size);
	char *boundary_end_pos = utils::strstr(buffer, boundary_end.c_str(), _buffer_size);
	if (boundary_pos && boundary_pos != boundary_end_pos) {
		write(file_fd, buffer, (boundary_pos - buffer) - 2);
		close(file_fd);
		deleteStream(client_socket);
		int readBodyResult = stream.req.readBodyFile(boundary_pos, _buffer_size - (boundary_pos - buffer), *this);
		if (readBodyResult != -1) {
			addStream(client_socket, readBodyResult, stream.req, boundary);
		}
	} else if (boundary_end_pos) {
		_buffer_size = (boundary_end_pos - buffer) + 2;
		write(file_fd, buffer, _buffer_size);
		close(file_fd);
		deleteStream(client_socket);
	} else {
		write(file_fd, buffer, _buffer_size);
	}
}

void Server::_processResponseStream(int client_socket)
{
	Outstream resp = _res_streams.find(client_socket)->second;
	ssize_t bytes_sent = send(client_socket, resp.buffer, resp.bytes_to_send, MSG_DONTWAIT);
	if (bytes_sent < 0)
	{
		if (resp.counter > MAX_NUBMER_ATTEMPTS) {
			_res_streams.erase(client_socket);
			std::cout << "Maximum number of send attempts excided" << std::endl;
			return;
		} else {
			resp.counter++;
		}
	}
	if (resp.bytes_to_send > bytes_sent) {
		char* buffer_to_save = resp.buffer + bytes_sent;
		Outstream outsteam(resp.bytes_to_send - bytes_sent, buffer_to_save, resp.status);
		_res_streams.erase(client_socket);
		_res_streams[client_socket] = outsteam;
	} else if (resp.bytes_to_send == bytes_sent) {
		_res_streams.erase(client_socket);
	}
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

void Server::setBuffer(char *buffer, int buffer_size)
{
	_buffer = buffer;
	_buffer_size = buffer_size;
}

void Server::setResBuffer(char *buffer, int buffer_size)
{
	_res_buffer = buffer;
	_res_buffer_size = buffer_size;
}

void Server::addStream(int client_socket, int file_fd, Request &req, std::string& boundary)
{
	Stream stream(req, boundary, file_fd);
	_streams[client_socket] = stream;
}

void Server::deleteStream(int client_socket)
{
	_streams.erase(client_socket);
}

void Server::initEndpoint(const HostList &hosts, short port, const ServerConfig &config) {
	_port = port;
	_hosts = hosts;
	_config = config;
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
	for (size_t i = 0; i < _fds.size(); ++i)
	{
		if (_fds[i].fd != _main_socketfd && _checkRequestTimeout(_fds[i].fd)) {
			// Response res(_config, 408, _res_buffer, _res_buffer_size);
			/* Debug print */
			// std::cout << CYAN << "Response sent:" << std::endl << response << RESET << std::endl;

			// send(_fds[i].fd, res.getContent(), res.getContentLength(), MSG_DONTWAIT);
			close(_fds[i].fd);
			_fds.erase(_fds.begin() + i);
			// _cleanChunkFiles(_fds[i].fd);
		}
	}

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
			close(_fds[i].fd);
			_fds.erase(_fds.begin() + i);
			if (_fds[i].fd == _main_socketfd)
				throw PollingErrorException("error from poll() function");
		}
		int client_socket = _fds[i].fd;
		if (_fds[i].revents & POLLIN) {
			if (client_socket != _main_socketfd)
				_setRequestTime(client_socket);
			if (client_socket == _main_socketfd) {
				_addNewClient(client_socket);
			} else {
				_serveExistingClient(client_socket, i);
			}
		} else if (_fds[i].revents & POLLOUT) {
			if (_res_streams.find(_fds[i].fd) != _res_streams.end())
				_processResponseStream(_fds[i].fd);
			// } else {
			// 	// _serveSendingResponse(client_socket, i);
			// }
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

void Server::_setRequestTime(int client_socket)
{
	_request_time[client_socket] = utils::getCurrentTime();
}

bool Server::_checkRequestTimeout(int client_socket)
{
	if (difftime(utils::getCurrentTime(), _request_time[client_socket]) >= REQUEST_TIMEOUT)
		return true;
	return false;
}

void Server::_cleanChunkFiles(int client_socket)
{
	if (utils::checkChunkFileExistance(utils::buildPath(client_socket, TEMP_FILES_DIRECTORY)))
		utils::deleteFile(utils::buildPath(client_socket, TEMP_FILES_DIRECTORY));
}

void Server::listenPort(int backlog) {
	if (listen(_main_socketfd, backlog) < 0) {
		ListenErrorException(strerror(errno));
	}
}

const HostList &Server::getHostList() const {
	return _hosts;
}

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
		{
			int buffer_size = servers[i]._config.body_limit + 10 * 1024;
			char *buffer = new char[buffer_size];
			servers[i].setBuffer(buffer, buffer_size);
		}
		{
			int buffer_size = RESPONSE_MAX_BODY_SIZE + 10 * 1024;
			char* buffer = new char[buffer_size];
			servers[i].setResBuffer(buffer, buffer_size);
		}
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

Outstream::Outstream(ssize_t bytes, const char *buffer, int status_code) {
	counter = 0;
	bytes_to_send = bytes;
    status = status_code;
	this->buffer = new char[bytes_to_send];
	std::memset(this->buffer, 0, bytes_to_send);
	memmove(this->buffer, buffer, bytes_to_send);
}

Outstream::Outstream(const Outstream &src)
{
	if (src.bytes_to_send > 0) {
		buffer = new char[src.bytes_to_send];
		std::memmove(buffer, src.buffer, src.bytes_to_send);
	}
	else
		buffer = 0;
	bytes_to_send = src.bytes_to_send;
	counter = src.counter;
    status = src.status;
}

Outstream &Outstream::operator=(const Outstream &src)
{
	if (this != &src) {
		if (buffer)
			delete[] buffer;
		buffer = new char[src.bytes_to_send];
		std::memmove(buffer, src.buffer, src.bytes_to_send);
		bytes_to_send = src.bytes_to_send;
		counter = src.counter;
        status = src.status;
	}
	return *this;
}
