#include "Response.hpp"
#include <fstream>
#include <sstream>

Response::Response(const Request& req, const ServerConfig& config)
	: _httpVersion("HTTP/1.1"), _config(config) {

    if (req.getMethod() == "GET")
        handleGetRequest(req);
    else if (req.getMethod() == "POST")
        handlePostRequest(req);
    else if (req.getMethod() == "DELETE")
        handleDeleteRequest(req);
    else
		_setError(405);
}

void Response::handleGetRequest(const Request& req) {
    std::string filename = _config.root + req.getUri();
    if (filename == _config.root + "/")
		filename = _config.root + "/index.html";

	std::cout << "Filename: " << filename << std::endl;

    std::string content = readFile(filename);
    if (content.empty()) {
		_setError(404);
    } else {
        setStatus(200, "OK");
        setBody(content);
        addHeader("Content-Type", getMimeType(filename));
    }
}

void Response::handlePostRequest(const Request& req) {
    setStatus(200, "OK");
    setBody("POST request received for URI: " + req.getUri() + "\nBody: " + req.getBody());
    addHeader("Content-Type", "text/plain");
}

void Response::handleDeleteRequest(const Request& req) {
    setStatus(200, "OK");
    setBody("DELETE request received for URI: " + req.getUri());
    addHeader("Content-Type", "text/plain");
}

void Response::setStatus(int code, const std::string& message) {
    _statusCode = code;
    _statusMessage = message;
}

void Response::addHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void Response::setBody(const std::string& body) {
    _body = body;
    addHeader("Content-Length", toString(_body.length()));
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
        setStatus(code, "OK");
        setBody(content);
        addHeader("Content-Type", getMimeType(filename));
    }

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
