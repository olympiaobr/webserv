#ifndef Server_HPP
# define Server_HPP

# include <poll.h>
# include <vector>
# include <unistd.h>
# include <netinet/in.h>
# include <iostream>
# include <stdio.h>
# include <string.h>
# include <sys/errno.h>
# include <fcntl.h>
# include <sstream>
# include <dirent.h>
# include <ctime>
# include <stdlib.h>
# include "../requests/Request.hpp"
# include "../configuration/Config.hpp"
# include "../responses/Response.hpp"
// #include "../responses/Response.hpp"

# include "../include/debug.hpp"


typedef std::vector<std::string> HostList;

# define BACKLOG 3
# define TEMP_FILES_DIRECTORY "tmp/"

class Server
{
	public:
		Server();
		Server(const HostList &hosts, short port);
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
		size_t		getSocketsSize() const;
		void		listenPort(int backlog);
		/* Getters */
		const HostList				&getHostList() const;
		short						getPort() const;
		int							getMainSocketFd() const;
		const std::vector<pollfd>	&getSockets() const;

		/* Server routines */

		/* Start all servers */
		static void RUN(std::vector<Server>);
	private:
		/*Variables*/
		short				_port;
		HostList			_hosts;
		int					_main_socketfd;
		sockaddr_in			_address;
		int					_address_len;
		std::vector<pollfd>	_fds;
		ServerConfig		_config;
		/*Functions*/
		void _push(pollfd client_pollfd); //called when setPollfd is called
		void _setSocketOpt();
		void _setSocketNonblock();
		void _bindSocketName();
		bool _chunkHandler(Request &req);
		void _processingRouter(Request &req);
		void _parseFormData(Request &req);
		void _parsePartialData(Request &req);
		std::string _saveFile(const std::string &file_name);
		std::string _chunkFileName(int socket);
};

std::ostream &operator<<(std::ostream &os, const Server &server);

#endif
