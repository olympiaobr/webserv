#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include <string>
#include <map>
#include <algorithm>
#include "../requests/Request.hpp"
#include "../configuration/Config.hpp"
#include "../utilities/Utils.hpp"

class Request;

class Response {

public:
	Response(const ServerConfig& config, char* buffer, int buffer_size);
	Response(const ServerConfig& config, int errorCode, char* buffer, int buffer_size);
    Response(const Request& req, const ServerConfig& config, char *buffer, int buffer_size);
	Response& operator=(const Response& other);

    void initializeHttpErrors();
    void setStatus(int code);
	int getStatusCode();
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    std::string toString() const;
	const char* toCString();

private:
    std::string 						_httpVersion;
	const ServerConfig&					_config;
    int									_statusCode;
    std::string							_statusMessage;
    std::map<std::string, std::string>	_headers;
    std::string							_body;
    std::string							_responseString;
    std::map<int, std::string>			_httpErrors;

	char*								_buffer;
	int									_buffer_size;

    std::string _readFile(const std::string& filename);
    std::string _getMimeType(const std::string& filename);
    std::string _toString(size_t num) const;
	std::vector<std::string> _chunkFile(const std::string& filename);

    void _handleGetRequest(const Request& req);
    void _handlePostRequest(const Request& req);
    void _handleDeleteRequest(const Request& req);
	void _setError(int code);
    const RouteConfig* _findMostSpecificRouteConfig(const std::string& uri) const;
    void _dispatchMethodHandler(const Request& req);

};

#endif
