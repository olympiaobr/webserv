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

# include "../configuration/Config.hpp"

struct Stream;
class Server;
class Session;

class Request {
public:
	Request();
	Request(int clientSocket);
	Request(const Request& src, size_t extend);
	~Request();

	Request& operator=(const Request& src);

	void parseHeaders();

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
    const RouteConfig* getRouteConfig() const;
	ServerConfig* getConfig() const;

	char* 		getBuffer() const;
	size_t		getBufferLen() const;

	void		setBufferLen(size_t len);

	std::string RemoveQueryString(std::string uri) const;

	void setSessionId(std::string sessionid);

	void readBodyChunked(char *init_buffer, ssize_t bytesRead);

	bool isTargetingCGI() const;

	// void addHeader(const std::string& key, const std::string& value);

	bool setConfig(std::vector<ServerConfig> &configs);

	friend std::ostream &operator<<(std::ostream &os, const Request &request);

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

	char *buffer;
	size_t buffer_length;
	int total_read;

private:
	std::string _session_id;
	int _clientSocket;
	std::string _method;
	std::string _uri;
	std::string _httpVersion;
	std::map<std::string, std::string> _headers;
	std::string _body;
	ServerConfig *_config;
	RouteConfig *_route_config;

	void _parseRequestLine(const std::string &line);
	void _parseHeader(const std::string &line);

	RouteConfig *_findMostSpecificRouteConfig(const std::string &uri);
};

#endif
