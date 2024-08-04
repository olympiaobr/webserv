#include "Response.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sys/stat.h>

Response::Response(const ServerConfig& config): _config(config), _statusCode(-1) {}

Response::Response(const ServerConfig& config, int errorCode)
	: _httpVersion("HTTP/1.1"), _config(config) {
	initializeHttpErrors();
	_setError(errorCode);
}


Response::Response(const Request& req, const ServerConfig& config)
    : _httpVersion("HTTP/1.1"), _config(config) {
    initializeHttpErrors();

    const RouteConfig* routeConfig = findMostSpecificRouteConfig(req.getUri());
    if (!routeConfig) {
        _setError(404);
        return;
    }
    if (std::find(routeConfig->allowed_methods.begin(), routeConfig->allowed_methods.end(), req.getMethod()) == routeConfig->allowed_methods.end()) {
        _setError(405);
        return;
    }
    dispatchMethodHandler(req);
}

const RouteConfig* Response::findMostSpecificRouteConfig(const std::string& uri) const {
    const RouteConfig* bestMatch = NULL;
    size_t longestMatchLength = 0;

    for (std::map<std::string, RouteConfig>::const_iterator it = _config.routes.begin(); it != _config.routes.end(); ++it) {
        const std::string& basePath = it->first;
        if (uri.find(basePath) == 0 && basePath.length() > longestMatchLength) {
            bestMatch = &it->second;
            longestMatchLength = basePath.length();
        }
    }
    return bestMatch;
}

void Response::dispatchMethodHandler(const Request& req) {
    if (req.getMethod() == "GET")
        handleGetRequest(req);
    else if (req.getMethod() == "POST")
        handlePostRequest(req);
    else if (req.getMethod() == "DELETE")
        handleDeleteRequest(req);
    else
        _setError(405);
}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		_httpVersion = other._httpVersion;
		_statusCode = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers = other._headers;
		_body = other._body;
		_responseString = other._responseString;
		_httpErrors = other._httpErrors;
	}
	return *this;
}

void Response::initializeHttpErrors() {
    _httpErrors[200] = "OK";
    _httpErrors[201] = "Created";
    // _httpErrors[300] = "Multiple Choices";
    _httpErrors[400] = "Bad Request";
    // _httpErrors[403] = "Forbidden";
    _httpErrors[404] = "Not Found";
    _httpErrors[405] = "Method Not Allowed";
    _httpErrors[408] = "Request Timeout";
    _httpErrors[413] = "Payload Too Large";
    _httpErrors[414] = "URI Too Long";
    _httpErrors[418] = "I'm a teapot";
    _httpErrors[500] = "Internal Server Error";
    // _httpErrors[501] = "Not Implemented";
    _httpErrors[503] = "Service Unavailable";
    _httpErrors[505] = "HTTP Version Not Supported";
}

void Response::handleGetRequest(const Request& req) {
    std::string filename = _config.root + req.getUri();

    if (req.getUri() == "/teapot") {
        _setError(418);
        return;
    }

    struct stat fileStat;
    if (stat(filename.c_str(), &fileStat) == 0 && S_ISDIR(fileStat.st_mode)) {

        filename += "/index.html";
        if (stat(filename.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode)) {
            std::string content = readFile(filename);
            if (content.empty()) {
                _setError(404);
            } else {
                setStatus(200);
                setBody(content);
                addHeader("Content-Type", getMimeType(filename));
            }
        }
        else {
            _setError(404);
        }
        return;
    }
    std::string content = readFile(filename);
    if (content.empty()) {
        _setError(404);
    } else {
        setStatus(200);
        setBody(content);
        addHeader("Content-Type", getMimeType(filename));
    }
}

void Response::handlePostRequest(const Request& req) {
    setStatus(200);
    setBody("POST request received for URI: " + req.getUri() + "\nBody: \n" + req.getBody());
    addHeader("Content-Type", "text/plain");
}

void Response::handleDeleteRequest(const Request& req) {
    setStatus(200);
    setBody("DELETE request received for URI: " + req.getUri());
    addHeader("Content-Type", "text/plain");
}

void Response::setStatus(int code) {
    _statusCode = code;
    std::map<int, std::string>::iterator it = _httpErrors.find(code);
    if (it != _httpErrors.end()) {
        _statusMessage = it->second;
    }
    else {
        std::ostringstream errMsg;
        errMsg << "Status code " << code << " not found in error map";
        throw std::runtime_error(errMsg.str());
    }
}

void Response::setBody(const std::string& body) {
    _body = body;
    addHeader("Content-Length", toString(_body.length()));
}

int Response::getStatusCode() {
	return _statusCode;
}

void Response::_setError(int code) {
	std::string path;
	std::map<int, std::string>::const_iterator it = _config.error_pages.find(code);

	if (it != _config.error_pages.end())
		path = it->second;
	else
		throw std::logic_error("Error code not found in configuration");
	std::string filename = _config.root + path;

    std::string content = readFile(filename);
    if (content.empty())
		throw std::logic_error("Error file not found");
    else {
        setStatus(code);
        setBody(content);
        addHeader("Content-Type", getMimeType(filename));
    }
}

void Response::addHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

std::string Response::toString() const {
    std::ostringstream oss;
    oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    oss << "\r\n" << _body;
    return oss.str();
}

const char* Response::toCString() {
    _responseString = toString();
    return _responseString.c_str();
}

std::string Response::readFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file) {
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

std::string Response::getMimeType(const std::string& filename) {
    if (filename.find(".html") != std::string::npos) return "text/html";
    if (filename.find(".css") != std::string::npos) return "text/css";
    if (filename.find(".js") != std::string::npos) return "application/javascript";
    if (filename.find(".ico") != std::string::npos) return "image/x-icon";
    return "text/plain";
}

std::string Response::toString(size_t num) const {
    std::ostringstream oss;
    oss << num;
    return oss.str();
}
