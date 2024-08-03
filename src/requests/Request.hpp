#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>
# include <iostream>

# include "../include/debug.hpp"
# include "../utilities/Utils.hpp"

# define CLIENT_MAX_BODY_SIZE 10000000 // temp
# define BUFFER_SIZE 1024

class Request {
private:
	int _clientSocket;
	std::string _method;
	std::string _uri;
	std::string _httpVersion;
	std::map<std::string, std::string> _headers;
	std::string _body;

	void _parseRequestLine(const std::string& line);
	void _parseHeader(const std::string& line);
	void _readBody(int contentLength, const std::string& initialData);
	void _readBodyChunked(const std::string& initialData);

public:
	Request(int clientSocket);
	~Request();

	void parse();

	std::string getMethod() const;
	std::string getUri() const;
	std::string getHttpVersion() const;
	std::string getHeader(const std::string& key) const;
	const std::map<std::string, std::string>& getHeaders() const;
	std::string getBody() const;
	bool getBadRequestFlag() const;

	void addHeader(const std::string& key, const std::string& value);

	friend std::ostream& operator<<(std::ostream& os, const Request& request);

	/* Exceptions */
	enum ErrorType {CONTENT_LENGTH, INTERRUPT, FILE_SYSTEM, SOCKET_CLOSED, BAD_REQUEST};
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
};

#endif