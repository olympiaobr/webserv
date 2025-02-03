#ifndef SESSION_HPP
# define SESSION_HPP

# include "../requests/Request.hpp"
# include "../responses/Response.hpp"
# include "../cgi/CGI.hpp"

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

		void recieveData();

		void newRequest(std::vector<ServerConfig> &configs);
		void parseBody();

		void sendResponse();

		int getSocket() const;

		void handleCGI(const std::string& script_path);
		Response& createErrorResponse(const std::string& error_msg, const ServerConfig* config);

		void ensureSession(Request& req);

	private:

		char		_sessionId[100];
		int			_client_socket;

		std::string generateSessionId();

	public:
};


#endif
