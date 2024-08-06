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
private:
    std::string _httpVersion;
	const ServerConfig& _config;
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _responseString;
    std::map<int, std::string> _httpErrors;

    std::string readFile(const std::string& filename);
    std::string getMimeType(const std::string& filename);
    std::string toString(size_t num) const;

    void handleGetRequest(const Request& req);
    void handlePostRequest(const Request& req);
    void handleDeleteRequest(const Request& req);
	void _setError(int code);
    const RouteConfig* findMostSpecificRouteConfig(const std::string& uri) const;
    void dispatchMethodHandler(const Request& req);

public:
	Response(const ServerConfig& config);
	Response(const ServerConfig& config, int errorCode);
    Response(const Request& req, const ServerConfig& config);
	Response& operator=(const Response& other);

    void initializeHttpErrors();
    void setStatus(int code);
	int getStatusCode();
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    std::string toString() const;
	const char* toCString();
};

#endif
