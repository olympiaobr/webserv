#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>
# include <iostream>
# include <cstring>
# include <sstream>
# include <unistd.h>
# include <cstdlib>
# include <sys/socket.h>
# include <fcntl.h>
# include <errno.h>

# include "../responses/Response.hpp"
# include "../configuration/Config.hpp"
# include "../cgi/CGI.hpp"
# include "../utilities/Utils.hpp"
# include "../server/Session.hpp"


struct Stream;
class Server;
class Session;

class Request {
public:
	Request(int clientSocket, ServerConfig &config, char *buffer, int buffer_len);
	Request();
	~Request();

	int parseHeaders(std::vector<Session>& sessions);
	int parseBody(int bytesRead, Server& server);
	int readBodyFile(char *init_buffer, ssize_t bytesRead, Server& server);

	std::string getMethod() const;
	std::string getUri() const;
	std::string getHost() const;
	std::string getHttpVersion() const;
	std::string getHeader(const std::string& key) const;
	const std::map<std::string, std::string>& getHeaders() const;
	std::string getBody() const;
	int			getSocket() const;
	std::string getQueryString() const;
	std::string getScriptPath() const;
	std::string getSession() const;

	std::string RemoveQueryString(std::string uri) const;
	bool isTargetingCGI() const;

	void addHeader(const std::string& key, const std::string& value);

	friend std::ostream& operator<<(std::ostream& os, const Request& request);

	/* Exceptions */
	enum ErrorType {CONTENT_LENGTH, BAD_REQUEST, FILE_SYSTEM, INTERRUPT};
	class ParsingErrorException: public std::exception {
		public:
			ParsingErrorException(ErrorType type, const char *error_msg);
			const char* what() const throw();
			ErrorType type;
		private:
			char _error[256];
	};
	class SocketCloseException: public std::exception {
		public:
			SocketCloseException(const char *error_msg);
			const char* what() const throw();
		private:
			char _error[256];
	};
	class StreamingData: public std::exception {
		public:
			const char* what() const throw();
	};


	private:
		std::string								_session_id;
		int 									_clientSocket;
		std::string								_method;
		std::string								_uri;
		std::string								_httpVersion;
		std::map<std::string, std::string>		_headers;
		std::string								_body;
		ServerConfig							_config;

		char*									_buffer;
		size_t									_buffer_size;


		void _parseRequestLine(const std::string& line);
		void _parseHeader(const std::string& line);
		void _readBody(const char *init_buffer, ssize_t bytesRead);
		void _readBodyChunked(const char *init_buffer, ssize_t bytesRead);

};

#endif
