#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include "../requests/Request.hpp"
#include "../configuration/Config.hpp"

class Response {
private:
    std::string _httpVersion;
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _responseString;
	const ServerConfig& _config;

    std::string readFile(const std::string& filename);
    std::string getMimeType(const std::string& filename);
    std::string toString(size_t num) const;

    void handleGetRequest(const Request& req);
    void handlePostRequest(const Request& req);
    void handleDeleteRequest(const Request& req);
	void _setError(int code);

public:
    Response(const Request& req, const ServerConfig& config);

    void setStatus(int code, const std::string& message);
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    std::string toString() const;
	const char* toCString();
};

#endif