#ifndef SESSION_HPP
# define SESSION_HPP

# include <poll.h>
# include <vector>
# include <unistd.h>

class Session
{
	public:
		Session();
		~Session();
		void addPollfd(int socket_fd, short events);
		void setMainSocketfd(int main_socketfd);
		void pollfds();
	private:
		std::vector<pollfd> _fds;
		int _main_socketfd;
		void _push(pollfd client_pollfd); //called when setPollfd is called
};


#endif
