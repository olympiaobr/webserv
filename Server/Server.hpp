#ifndef Server_HPP
# define Server_HPP

# include <poll.h>
# include <vector>
# include <unistd.h>
# include <netinet/in.h>
# include <iostream>

class Server
{
	public:
		Server();
		~Server();
		/*Exceptions*/
		class PollingErrorException: public std::exception
		{
			private:
				char _error[256];
			public:
				PollingErrorException(const char *error_place);
				const char* what() const throw();
		};
		void initMainSocket(int main_socketfd, sockaddr_in address);
		/* Socket fucntions */
		void addPollfd(int socket_fd, short events);
		void pollfds();
		/* poll listen loop */
		void pollLoop();
		/* Utility functions */
		size_t socketsSize();
	private:
		std::vector<pollfd>	_fds;
		int					_main_socketfd;
		sockaddr_in			_address;
		int					_address_len;
		void _push(pollfd client_pollfd); //called when setPollfd is called
};


#endif
