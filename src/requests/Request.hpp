#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>
# include <iostream>
# include <string.h>

# include "../server/Server.hpp"
# include "../include/debug.hpp"
# include "../utilities/Utils.hpp"
# include "../configuration/Config.hpp"

// # define CLIENT_MAX_BODY_SIZE 10000 // temp
// # define BUFFER_SIZE 10000


class Request {
public:
	Request(int clientSocket, ServerConfig &config);
	~Request();

	int parseHeaders(char *buffer, int buffer_size);
	int parseBody(char *buffer, int buffer_size, int bytesRead);

	std::string getMethod() const;
	std::string getUri() const;
	std::string getHost() const;
	std::string getHttpVersion() const;
	std::string getHeader(const std::string& key) const;
	const std::map<std::string, std::string>& getHeaders() const;
	std::string getBody() const;
	int			getSocket() const;

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

	private:
	int _clientSocket;
	std::string _method;
	std::string _uri;
	std::string _httpVersion;
	std::map<std::string, std::string> _headers;
	std::string _body;
	ServerConfig _config;


	void _parseRequestLine(const std::string& line);
	void _parseHeader(const std::string& line);
	void _readBody(const char *init_buffer, ssize_t bytesRead);
	void _readBodyChunked(const char *init_buffer, ssize_t bytesRead);
	void _readBodyFile(const char *init_buffer, ssize_t bytesRead);

};

#endif
