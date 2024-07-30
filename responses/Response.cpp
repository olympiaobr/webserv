#include "Response.hpp"
#include <sstream>

Response::Response() 
    : _statusCode(200), 
      _statusMessage("OK"), 
      _contentType("text/plain"),
      _content("") {}

Response::Response(int statusCode, const std::string& statusMessage, const std::string& content, const std::string& contentType)
    : _statusCode(statusCode), 
      _statusMessage(statusMessage), 
      _contentType(contentType),
      _content(content) {
    _buildBuffer();
}

void Response::setStatus(int statusCode, const std::string& statusMessage) {
    _statusCode = statusCode;
    _statusMessage = statusMessage;
    _buildBuffer();
}

void Response::setContent(const std::string& content) {
    _content = content;
    _buildBuffer();
}

void Response::setContentType(const std::string& contentType) {
    _contentType = contentType;
    _buildBuffer();
}

void Response::_buildBuffer() {
    std::ostringstream response;
    response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
    response << "Content-Type: " << _contentType << "\r\n";
    response << "Content-Length: " << _content.length() << "\r\n";
    response << "\r\n";
    response << _content;

    _buffer = response.str();
}

const char* Response::build() {
    return _buffer.c_str();
}

size_t Response::getSize() const {
    return _buffer.size();
}