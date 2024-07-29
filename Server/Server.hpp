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

class Server
{
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
		/* Initialize server */
		void initEndpoint(short port);
		/* Socket fucntions */
		void addPollfd(int socket_fd, short events);
		void pollfds();
		/* poll listen loop */
		void pollLoop();
		/* Utility functions */
		size_t socketsSize();
		void listenPort(int backlog);
	private:
		std::vector<pollfd>	_fds;
		int					_main_socketfd;
		sockaddr_in			_address;
		int					_address_len;

		void _push(pollfd client_pollfd); //called when setPollfd is called
		void setSocketOpt();
		void setSocketNonblock();
		void bindSocketName();
};


#endif
