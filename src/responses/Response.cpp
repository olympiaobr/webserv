#include "Response.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>

Response::Response() : _httpVersion("HTTP/1.1"), _statusCode(200), _statusMessage("OK") {
    initializeHttpErrors();
}

Response::Response(const Request& req, const ServerConfig& config) : _httpVersion("HTTP/1.1") {
    initializeHttpErrors();
    routeRequest(req, config);
}

void Response::initializeHttpErrors() {
    _httpErrors[200] = "OK";
    _httpErrors[201] = "Created";
    _httpErrors[300] = "Multiple Choices";
    _httpErrors[403] = "Forbidden";
    _httpErrors[404] = "Not Found";
    _httpErrors[405] = "Method Not Allowed";
    _httpErrors[408] = "Request Timeout";
    _httpErrors[413] = "Payload Too Large";
    _httpErrors[414] = "URI Too Long";
    _httpErrors[418] = "I'm a teapot";
    _httpErrors[500] = "Internal Server Error";
    _httpErrors[501] = "Not Implemented";
    _httpErrors[503] = "Service Unavailable";
    _httpErrors[505] = "HTTP Version Not Supported";
}

void Response::routeRequest(const Request& req, const ServerConfig& config){
    if (req.getMethod() == "GET") {
        handleGetRequest(req, config);
    }
    else if (req.getMethod() == "POST") {
        handlePostRequest(req);
    }
    else if (req.getMethod() == "DELETE") {
        handleDeleteRequest(req);
    }
    else {
        /* Temporary ddavlety 03.08 */
        // setStatus(405);
        // setBody("405 - Method Not Allowed");
        setStatus(200);
        setBody("200 - file recieved");
        /* ddavlety 03.08 */
    }
}

void Response::handleGetRequest(const Request& req, const ServerConfig& config) {
    std::string filename = config.root + req.getUri();

    if (req.getUri() == "/teapot") {
        setStatus(418);
        setBody("418 - I'm a teapot");
        return;
    }

    if (filename == config.root + "/") {
        filename = config.root + "/index.html";
    }
    std::string content = readFile(filename);
    if (content.empty()) {
        setStatus(404);
        setBody("404 - Page not found");
    } else {
        setStatus(200);
        setBody(content);
        addHeader("Content-Type", getMimeType(filename));
    }
}

void Response::handlePostRequest(const Request& req) {
    setStatus(200);
    setBody("POST request received for URI: " + req.getUri() + "\nBody: " + req.getBody());
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
