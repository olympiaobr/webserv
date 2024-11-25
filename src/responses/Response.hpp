#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <string>
# include <map>
# include <algorithm>

# include "../utilities/Utils.hpp"
# include "../configuration/Config.hpp"
# include "../requests/Request.hpp"
# include "../cgi/CGI.hpp"

#define RESPONSE_MAX_BODY_SIZE 8 * 1024 * 1024
class Response {

public:
	Response();
	~Response();
	Response& operator=(const Response& other);

    void			initializeHttpErrors();
    void			setStatus(int code);
	int				getStatusCode();
    void			addHeader(const std::string& key, const std::string& value);

	void			setConfig(ServerConfig &config);

	void			generateResponse(const std::string& filename);
	void			generateDirectoryListing(const std::string& directoryPath);
	void			generateCGIResponse(const std::string &cgi_response);

	const char*		getContent();
	ssize_t			getContentLength();
	void			setContent(ssize_t move);

	void			initialize(const Request& req);


	/* Exceptions */
	class FileSystemErrorException: public std::exception {
		public:
			FileSystemErrorException(const char *error_msg);
			const char* what() const throw();
		private:
			char _error[256];
	};
	class ContentLengthException: public std::exception {
		public:
			ContentLengthException(const char *error_msg);
			const char* what() const throw();
		private:
			char _error[256];
	};

private:
    std::string 						_httpVersion;
	ServerConfig*						_config;
    int									_statusCode;
    std::string							_statusMessage;
    std::map<std::string, std::string>	_headers;
    std::map<int, std::string>			_httpErrors;
    const char*							_content;

	char*								_buffer;
	size_t								_buffer_size;
	ssize_t								_content_length;
    ssize_t                             _headers_length;
	std::string 						_connection;

    std::string		_getMimeType(const std::string& filename);
    std::string		_toString(size_t num) const;

    void				_handleGetRequest(const Request& req, const RouteConfig* route_config);
    void				_handlePostRequest(const Request& req, const RouteConfig* route_config);
    void				_handleDeleteRequest(const Request& req, const RouteConfig* route_config);
	void				_setError(int code);
    void				_dispatchMethodHandler(const Request& req, const RouteConfig* route_config);
    std::string			_headersToString() const;
	bool _handleRedir(int redirect_status_code, const std::string& redirect_url);

};

#endif
