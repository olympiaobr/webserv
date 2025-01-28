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

# include "../configuration/Config.hpp"
# include "./Session.hpp"
# include "../utilities/Utils.hpp"
# include "../cgi/CGI.hpp"
# include "../include/debug.hpp"
# include "../requests/Request.hpp"

class Request;
class Response;


# define BACKLOG 3
# define REQUEST_TIMEOUT 5
# define MAX_NUBMER_ATTEMPTS 3
# define TEMP_FILES_DIRECTORY "tmp/"

typedef std::vector<std::string> HostList;

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
		void initEndpoint(short port, const std::vector<ServerConfig> &configs);
		/* Socket fucntions */
		void	addPollfd(int socket_fd, short events);
		void	pollfds();
		void	pollLoop();

		/* Utility functions */
		size_t	getSocketsSize() const;
		void	listenPort(int backlog);
		/* Getters */
		// const HostList				&getHostList() const;
		short						getPort() const;
		int							getMainSocketFd() const;
		const std::vector<pollfd>	&getSockets() const;

		/* Start all servers */
		static void					RUN(std::vector<Server>);

	private:
		/*Variables*/
		short						_port;
		int							_main_socketfd;
		sockaddr_in					_address;
		int							_address_len;
		std::vector<pollfd>			_fds;
		std::vector<ServerConfig>	_configs;
		std::map<int, std::time_t>	_request_time;

		std::map<int, Session>		_sessions;

		/*Functions*/
		void _push(pollfd client_pollfd); //called when setPollfd is called
		void _setSocketOpt();
		void _setSocketNonblock();
		void _bindSocketName();

		void _cleanChunkFiles(int client_socket);

		/* Loop methods */
		void _addNewClient(int client_socket);
		void _serveExistingClient(Session &client, size_t i);
		void _processRequest(Session &client, size_t i);
		void _sendResponse(Session& client, size_t i);
};

std::ostream &operator<<(std::ostream &os, const Server &server);

#endif
