#include "Server.hpp"
#include "../responses/Response.hpp"
#include "../requests/Request.hpp"

Server::Server() {
	/* Clean up temp files before server start up */
	/* If there is already running server behaviour is undefined */
	DIR* dir = opendir(TEMP_FILES_DIRECTORY);
	if (dir == nullptr) {
		std::string error_msg;
		error_msg += strerror(errno);
		error_msg += ": failed to open directory";
		throw InitialisationException(error_msg.c_str());
	}
	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		if (entry->d_type == DT_REG) {
			std::string filePath = std::string(TEMP_FILES_DIRECTORY) + entry->d_name;
			/* IMPORTANT! remove function may be not allowed */
			remove(filePath.c_str());
		}
	}
	closedir(dir);
}

Server::Server(const HostList &hosts, short port): _port(port), _hosts(hosts) {
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
*	If event happend its revents is set accordingly.
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
				int new_client_socket = accept(_main_socketfd, (struct sockaddr *)&_address, (socklen_t*)&_address_len);
				if (new_client_socket < 0) {
						throw PollingErrorException(strerror(errno));
					}
				addPollfd(new_client_socket, POLLIN);
				std::cout << "New connection established on fd: " << client_socket << std::endl;
			} else {										//if it is existing connection
				/* Request */
				Request req(client_socket);
				try {
					req.parse();
				} catch (Request::SocketCloseException &e) {
					/* If socket closed by client we erase socket from polling list */
					std::cout << e.what() << std::endl;
					/* Delete .chunk file if exists */
					if (utils::checkChunkFileExistance(utils::buildPath(client_socket, TEMP_FILES_DIRECTORY)))
						utils::deleteFile(utils::buildPath(client_socket, TEMP_FILES_DIRECTORY));
					_fds.erase(_fds.begin() + i);
					continue ;
				}
				/* Debug print */
				std::cout << YELLOW << req << RESET << std::endl;
				/* Chunk handling */
				chunkHandler(req, client_socket);
				/* Response */
				Response res(req, _config);
				const char* response = res.toCString();
				std::cout << CYAN << "Response sent:" << std::endl << response << RESET << std::endl;
				send(client_socket, response, strlen(response), 0);
			}
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

std::string Server::_saveFile(const std::string &file_name)
{
	std::time_t now = std::time(nullptr);
	std::tm* now_tm = std::localtime(&now);

	std::ostringstream oss;
	oss << (now_tm->tm_year + 1900)
        << (now_tm->tm_mon + 1)
        << now_tm->tm_mday
        << now_tm->tm_hour
        << now_tm->tm_min
        << now_tm->tm_sec;

	std::string new_file_name;
	new_file_name += _config.root;
	new_file_name += "/";
	new_file_name += "uploads/";
	new_file_name += oss.str();
	new_file_name += "-";
	oss.clear();
	oss << rand() % 1000;
	new_file_name += oss.str();
	new_file_name += ".file";

	rename(file_name.c_str(), new_file_name.c_str());
	return new_file_name;
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

bool Server::chunkHandler(Request &req, int client_socket) {
	if (req.getHeader("Transfer-Encoding") == "chunked") {
		/* Get data size */
		std::stringstream ss;
		size_t len;
		std::string data;
		std::string file_name;
		std::string socket_name;

		file_name = TEMP_FILES_DIRECTORY;
		ss << client_socket;
		ss >> socket_name;
		ss.clear();
		file_name += socket_name;
		file_name += ".chunk";
		if (!access(file_name.c_str(), W_OK | R_OK))
			std::cout << BLACK << "File exists with permissions" << RESET << std::endl; // file exists, but it is from previous request?
		int file_fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0600);
		if (!file_fd)
			throw RuntimeErrorException("error opening the file");
		data = req.getBody();
		while (data != "")
		{
			std::string tmp;
			tmp = data;
			size_t end = data.find("\r\n");
			data = data.substr(0, end);
			ss << std::hex << data;
			ss >> len;
			ss.clear();
			/* If end of chunks (save file) */
			if (len == 0) {
				close(file_fd);
				req.addHeader("Transfer", "finished");
				_saveFile(file_name);
				return true;
			}
			/* Get data */
			data = tmp;
			size_t end_data = data.find("\r\n", end + 1);
			end += 2;
			end_data -= end;
			data = data.substr(end, end_data);
			if (write(file_fd, data.c_str(), len) < 0) // len or end_data??
				throw RuntimeErrorException("error writing to file"); // throw error
			end_data += 2 + end;
			data = tmp.substr(end_data);
		}
		close(file_fd);
	} else {
		return true;
	}
	req.addHeader("Transfer", "in progress");
	return false;
}

void Server::RUN(std::vector<Server> servers) {
	for (size_t i = 0; i < servers.size(); ++i) {
		servers[i].listenPort(BACKLOG);
		std::cout << "Server 1 is listening on port "
			<< servers[i].getPort() << std::endl;
	}
	while (true)
	{
		for (size_t i = 0; i < servers.size(); ++i) {
			servers[i].pollfds();
			servers[i].pollLoop();
		}
	}
}

/*Exceptions*/

Server::PollingErrorException::PollingErrorException(const char *error_msg) {
	strncpy(_error, "Pooling error: ", 15);
	_error[sizeof(_error) - 1] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::PollingErrorException::what() const throw() {
	return _error;
}

Server::InitialisationException::InitialisationException(const char *error_msg) {
	strncpy(_error, "Pooling error: ", 15);
	_error[sizeof(_error) - 1] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::InitialisationException::what() const throw() {
	return _error;
}

Server::ListenErrorException::ListenErrorException(const char *error_msg) {
	strncpy(_error, "Pooling error: ", 15);
	_error[sizeof(_error) - 1] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Server::ListenErrorException::what() const throw() {
	return _error;
}

Server::RuntimeErrorException::RuntimeErrorException(const char *error_msg) {
	strncpy(_error, "Runtime error: ", 15);
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
