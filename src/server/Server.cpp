#include "Server.hpp"
#include <signal.h>
int g_sig = 0;


void signal_handler(int sig) {
  g_sig = sig;
  signal(SIGINT, SIG_DFL);  // Restore default behavior
}

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
		if (client.status == client.S_REQUEST) {
			if (client.request.total_read > client.request.getRouteConfig()->body_limit)
				throw Request::ParsingErrorException(Request::CONTENT_LENGTH, "content length is above the limit");
			if (client.request.getHeader("Transfer-Encoding") == "chunked") {
				try {
					client.request.readBodyChunked(client.request.getBuffer(), client.request.total_read);
					// If we get here and found a zero chunk, we're done
					if (client.request.total_read == 0) {
						client.status = client.S_PROCESS;
					}
				} catch (const Request::ParsingErrorException& e) {
					throw;
				}
			} else if (client.request.total_read >= atoi(client.request.getHeader("content-length").c_str())) {
				client.status = client.S_PROCESS;
			}
			std::cout << "recieved more data" << std::endl;
		}
	} catch (Request::SocketCloseException &e) {
		_cleanChunkFiles(client_socket);
		close(client_socket);
		_fds.erase(_fds.begin() + i);
		_sessions.erase(client_socket);
		std::cout << "socket closed on " << client_socket << std::endl;
		return ;
	} catch (Request::ParsingErrorException& e) {
		client.status = client.S_RESPONSE;
		if (e.type == Request::BAD_REQUEST) {
			if (res.getConfig() == NULL) {
				_cleanChunkFiles(client_socket);
				close(client_socket);
				_fds.erase(_fds.begin() + i);
				_sessions.erase(client_socket);
				std::cout << "socket closed on " << client_socket << std::endl;
				return;
			}
			res.setError(400);
		}
		else if (e.type == Request::CONTENT_LENGTH)
			res.setError(413);
		else if (e.type == Request::FILE_SYSTEM)
			res.setError(500);
		_cleanChunkFiles(client_socket);
	}
	/*******************/
	int response_code = res.getStatusCode();
	if (response_code <= -1)
		res.setError(500);
	if (response_code >= 500 || response_code >= 400) {
		_cleanChunkFiles(client_socket);
	}
}

bool isValidPath(const std::string& path)
{
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        if (!isalnum(c) && c != '_' && c != '/' && c != '.') {
            return false;
        }
    }
    return true;
}

void Server::_processRequest(Session &client, size_t i)
{
    (void)i;
    Request& req = client.request;
    Response& res = client.response;

    try {
        if (req.isTargetingCGI()) {
            std::string scriptPath = req.getScriptPath();
            if (!utils::fileExists(scriptPath)) {
                res.setError(404);
            } else if (!isValidPath(scriptPath)) {
                res.setError(404);
            } else {
                CGIHandler cgiHandler(scriptPath, req);
                std::string cgiOutput = cgiHandler.execute();
                if (cgiOutput.empty()) {
                    res.setError(500);
                } else {
                    res.setStatus(200);
                    res.addHeader("Content-Type", "text/html");
                    res.addHeader("Set-Cookie", req.getSession());
                    res.generateCGIResponse(cgiOutput);
                }
            }
        } else {
            res.initialize(req);
        }
    } catch (const Request::ParsingErrorException &e) {
        std::cerr << "Parsing error: " << e.what() << std::endl;
        res.setError(400);
    } catch (std::runtime_error &e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        res.setError(500);
    }

    client.status = client.S_RESPONSE;
}


void Server::_sendResponse(Session &client, size_t i)
{
	(void)i;

	client.sendResponse();
	if (client.response.getContentLength() == 0)
		client.status = client.S_DONE;
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
			_sessions.erase(_fds[i].fd);
			_fds.erase(_fds.begin() + i);
			if (_fds[i].fd == _main_socketfd)
				throw PollingErrorException("error from poll() function");
		}
		int client_socket = _fds[i].fd;
		if ((_fds[i].revents & POLLIN) || (client_socket != _main_socketfd && _sessions[client_socket].status == _sessions[client_socket].S_NEW)) {
			if (client_socket == _main_socketfd) {
				_addNewClient(client_socket);
			} else {
				_serveExistingClient(_sessions[client_socket], i);
			}
		} else if (_fds[i].revents & POLLOUT) {
			Session &client = _sessions[client_socket];
			if (client.status == client.S_PROCESS)
				_processRequest(client, i);
			if (client.status == client.S_RESPONSE)
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
  signal(SIGINT, signal_handler);
	for (size_t i = 0; i < servers.size(); ++i) {
		servers[i].listenPort(BACKLOG);
		std::cout << BLUE << "Server " << i + 1 << " is listening on port "
			<< servers[i].getPort() << RESET << std::endl;
	}
	while (true && !g_sig)
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
	os << "Server sockets:";
	if (server.getSocketsSize() == 0)
		os << "empty" << std::endl;
	for (size_t i = 0; i < server.getSocketsSize(); ++i)
		os << server.getSockets()[i].fd << std::endl;
	os << "end of server data" << std::endl;
	os << "------------------------------" << std::endl;
	return os;
}
