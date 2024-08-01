#include "Request.hpp"
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

Request::Request(int clientSocket) : _clientSocket(clientSocket) {}

Request::~Request() {}

bool Request::parse() {
    char buffer[1024];
    std::string request;
    int bytesRead;
    bool headersComplete = false;
    size_t headerEnd;

    // Set socket to non-blocking mode
    fcntl(_clientSocket, F_SETFL, O_NONBLOCK);

    while (true) {
        // Read from socket until the end of the headers is found
        bytesRead = recv(_clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            request += buffer;
            headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headersComplete = true;
                break;
            }
        } else if (bytesRead == 0) {
            throw ParsingErrorException(INTERRUPT, "Client closed the connection unexpectedly.");
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        } else {
            throw ParsingErrorException(INTERRUPT, "Socket error occurred.");
        }
    }
    if (!headersComplete) {
        std::cerr << "Incomplete headers" << std::endl;
        return false;
    }

    // Parse headers
    std::istringstream requestStream(request.substr(0, headerEnd));
    std::string line;
    if (std::getline(requestStream, line)) {
        _parseRequestLine(line);
    }
    while (std::getline(requestStream, line) && line != "\r") {
        _parseHeader(line);
    }

    // Handle body
    std::map<std::string, std::string>::const_iterator it = _headers.find("Content-Length");
    if (it != _headers.end()) {
        int contentLength = atoi(it->second.c_str());
		if (contentLength > CLIENT_MAX_BODY_SIZE)
			throw ParsingErrorException(CONTENT_LENGTH, "content length is above limit");
        std::string initialBodyData = request.substr(headerEnd + 4); // +4 to skip "\r\n\r\n"
        _readBody(contentLength, initialBodyData); // works incorect with some types of data in body
    } else {
        /* Chunked data recieved with no Content-lenght */
    }

    return true;
}

void Request::_parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    iss >> _method >> _uri >> _httpVersion;
}

void Request::_parseHeader(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        size_t first = value.find_first_not_of(" \t");
        size_t last = value.find_last_not_of(" \t\r\n");
        if (first != std::string::npos && last != std::string::npos) {
            value = value.substr(first, last - first + 1);
        }
        _headers[key] = value;
    }
}

void Request::_readBody(int contentLength, const std::string& initialData) {
    _body = initialData;
    int remainingBytes = contentLength - initialData.length();

    while (remainingBytes > 0) {
        char buffer[1024];
        int bytesRead = recv(_clientSocket, buffer, std::min(remainingBytes, (int)sizeof(buffer) - 1), 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            _body += buffer;
            remainingBytes -= bytesRead;
        } else if (bytesRead == 0) {
            throw ParsingErrorException(INTERRUPT, "Client closed the connection during body read.");
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Non-blocking read might return this if no data is available
            continue;
        } else {
            throw ParsingErrorException(INTERRUPT, "Error occurred while reading the body.");
        }
    }

    std::cout << "Total body length: " << _body.length() << std::endl;
}


std::string Request::getMethod() const {
    return _method;
}

std::string Request::getUri() const {
    return _uri;
}

std::string Request::getHttpVersion() const {
    return _httpVersion;
}

std::string Request::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

const std::map<std::string, std::string>& Request::getHeaders() const {
    return _headers;
}

std::string Request::getBody() const {
    return _body;
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
    os << "HTTP Request Details:" << std::endl;
    os << "Method: " << request.getMethod() << std::endl;
    os << "URI: " << request.getUri() << std::endl;
    os << "HTTP Version: " << request.getHttpVersion() << std::endl;
    os << "Headers:" << std::endl;

	std::map<std::string, std::string> headers = request.getHeaders();

	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it) {
		os << " " << it->first << ": " << it->second << std::endl;
	}
    os << "Body: " << request.getBody() << std::endl;
    return os;
}

Request::ParsingErrorException::ParsingErrorException(ErrorType type, const char *error_msg) {
	_type = type;
    strncpy(_error, "Request parsing exception: ", 27);
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Request::ParsingErrorException::what() const throw() {
	return _error;
}
