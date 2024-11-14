#ifndef SESSION_HPP
# define SESSION_HPP

# include <iostream>
# include "../utilities/Utils.hpp"
# include "./Server.hpp"

// class Request;
// class Response;

class Session
{
	private:
		/* Main data */

		/* Additinal data */
		char		_sessionId[100];
		std::string	_first_socket;
	public:
		Session();
		Session(int socket_id);
		Session(const Session& src);
		Request		request;
		Response	response;
		int			client_id;
		const char*	getSessionId() const;
		static std::vector<Session>::const_iterator findSession(std::vector<Session>& sessions, std::string& id);
};


#endif
