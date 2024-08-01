#include "Response.hpp"
#include <fstream>
#include <sstream>
 
Response::Response() : _httpVersion("HTTP/1.1"), _statusCode(200), _statusMessage("OK") {}

Response::Response(const Request& req) : _httpVersion("HTTP/1.1") {
    if (req.getMethod() == "GET") {
        handleGetRequest(req);
    } else if (req.getMethod() == "POST") {
        handlePostRequest(req);
    } else if (req.getMethod() == "DELETE") {
        handleDeleteRequest(req);
    } else {
        setStatus(405, "Method Not Allowed");
        setBody("Only GET, POST, and DELETE methods are supported");
    }
}

void Response::handleGetRequest(const Request& req) {
    std::string basePath = "./src/web";
    std::string requestedPath = req.getUri();

    if (requestedPath == "/" || requestedPath.empty()) {
        requestedPath += "index.html";
    }

    std::string fullPath = basePath + (requestedPath[0] == '/' ? "" : "/") + requestedPath;

    std::ifstream file(fullPath.c_str());
    if (!file) {
        setStatus(404, "Not Found");
        setBody("404 - Page not found");
    } else {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        setStatus(200, "OK");
        setBody(content);
        addHeader("Content-Type", getMimeType(fullPath));
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
    return "text/plain";
}

std::string Response::toString(size_t num) const {
    std::ostringstream oss;
    oss << num;
    return oss.str();
}
