#ifndef SESSION_HPP
# define SESSION_HPP

# include "../requests/Request.hpp"
# include "../responses/Response.hpp"
// # include "../utilities/Utils.hpp"
// # include "./Server.hpp"

// class Request;
// class Response;

class Session
{
	public:
		enum StatusType
		{
			S_NEW,
			S_REQUEST,
			S_PROCESS,
			S_RESPONSE,
			S_DONE
		};
		Request		request;
		Response	response;
		int			client_id;
		StatusType	status;

		const char*	getSessionId() const;

		Session();
		Session(int socket_id);
		Session(const Session& src);
		Session& operator=(const Session& src);
		// static std::vector<Session>::const_iterator findSession(std::vector<Session>& sessions, std::string& id);

		void recieveData();

		void newRequest(std::vector<ServerConfig> &configs);
		void parseBody();

		void sendResponse();

		int getSocket() const;

	private:

		char		_sessionId[100];
		int			_client_socket;
};


#endif
