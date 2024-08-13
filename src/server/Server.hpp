#ifndef SERVER_HPP
# define SERVER_HPP

# include <poll.h>
# include <vector>
# include <unistd.h>
# include <netinet/in.h>
# include <iostream>
# include <sys/errno.h>
# include <fcntl.h>
# include <sstream>
# include <ctime>
# include <cstdio>

# include "../requests/Request.hpp"
# include "../responses/Response.hpp"
# include "../configuration/Config.hpp"
# include "../cgi/CGI.hpp"
# include "../utilities/Utils.hpp"

# include "../include/debug.hpp"

class Request;
class Response;

typedef std::vector<std::string> HostList;

# define BACKLOG 3
// # define TEMP_FILES_DIRECTORY "tmp/"
# define REQUEST_TIMEOUT 10
# define RESPONSE_MAX_BODY_SIZE 80000000

struct Stream {
	int			counter;
	int			file_fd;
	Request		req;
	std::string	boundary;
	Stream(Request& req, std::string boundary, int file_fd): file_fd(file_fd), req(req), boundary(boundary) {};
	Stream(): req(Request()) {};
};

struct Outstream {
	int			counter;
	char*		buffer;
	ssize_t		bytes_to_send;
	Outstream(ssize_t bytes_to_send, char *buffer);
	Outstream(): buffer(0), bytes_to_send(-1) {};
	// ~Outstream() {if (buffer) delete[] buffer;};
	Outstream& operator=(const Outstream& src);
};

class Server {
	public:
		Server();
		~Server();

		/*Exceptions*/
		class PollingErrorException: public std::exception {
			private:
				char _error[256];
			public:
				PollingErrorException(const char *error_msg);
				const char* what() const throw();
		};
		class InitialisationException: public std::exception {
			private:
				char _error[256];
			public:
				InitialisationException(const char *error_msg);
				const char* what() const throw();
		};
		class ListenErrorException: public std::exception {
			private:
				char _error[256];
			public:
				ListenErrorException(const char *error_msg);
				const char* what() const throw();
		};
		class RuntimeErrorException: public std::exception {
			private:
				char _error[256];
			public:
				RuntimeErrorException(const char *error_msg);
				const char* what() const throw();
		};
		/* Initialize server */
		void	initEndpoint(const HostList &hosts, short port, const ServerConfig &config);
		/* Socket fucntions */
		void	addPollfd(int socket_fd, short events);
		void	pollfds();
		void	pollLoop();

		/* Utility functions */
		size_t	getSocketsSize() const;
		void	listenPort(int backlog);
		void	setBuffer(char *buffer, int buffer_size);
		void	setResBuffer(char *buffer, int buffer_size);
		void	addStream(int client_socket, int file_fd, Request& req, std::string& boundary);
		void	deleteStream(int client_socket);
		/* Getters */
		const HostList				&getHostList() const;
		short						getPort() const;
		int							getMainSocketFd() const;
		const std::vector<pollfd>	&getSockets() const;

		/* Start all servers */
		static void					RUN(std::vector<Server>);

	private:
		/*Variables*/
		short						_port;
		HostList					_hosts;
		int							_main_socketfd;
		sockaddr_in					_address;
		int							_address_len;
		std::vector<pollfd>			_fds;
		ServerConfig				_config;
		std::map<int, std::time_t>	_request_time;

		char*						_buffer;
		size_t						_buffer_size;
		char*						_res_buffer;
		size_t						_res_buffer_size;
		std::map<int, Stream>		_streams;
		std::map<int, Outstream>	_res_streams;

		/*Functions*/
		void _push(pollfd client_pollfd); //called when setPollfd is called
		void _setSocketOpt();
		void _setSocketNonblock();
		void _bindSocketName();
		void _setRequestTime(int client_socket);
		bool _checkRequestTimeout(int client_socket);
		void _cleanChunkFiles(int client_socket);
		void _addNewClient(int client_socket);
		void _requestHandling(Request &req, Response &res);
		void _serveExistingClient(int client_socket, size_t i);

		void _processStream(Stream stream);
		void _processResponseStream(int client_socket);
};

std::ostream &operator<<(std::ostream &os, const Server &server);

#endif
