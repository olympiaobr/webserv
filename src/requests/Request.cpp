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

void Request::parse() {
    char buffer[BUFFER_SIZE];
    std::string request;
    int bytesRead;
    bool headersComplete = false;
    size_t headerEnd;

    // Set socket to non-blocking mode


    while (true) {
        // Read from socket until the end of the headers is found
        bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            request += buffer;
            headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headersComplete = true;
                break;
            } else {
                /* Second part of request comes with no header */
                headersComplete = false;
                break;
            }
        } else if (bytesRead == 0) {
            throw SocketCloseException("connection closed by client");
        } else {
            throw ParsingErrorException(INTERRUPT, strerror(errno));
        }
    }

    if (headersComplete) {
        /* Parse headers only if it is complete */
        /* If header is incomplete, request is second part of initial request */
        // Parse headers
        std::istringstream requestStream(request.substr(0, headerEnd));
        std::string line;
        if (std::getline(requestStream, line)) {
            _parseRequestLine(line);
        }
        while (std::getline(requestStream, line) && line != "\r") {
            _parseHeader(line);
        }
    }

	/* Debug print */
	std::map<std::string, std::string> header = 	getHeaders();
	std::cout << MAGENTA << "Headers: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it=header.begin(); it != header.end(); ++it) {

		std::cout << it->first << ": " << it->second << std::endl;
	}
	std::cout << RESET;
	/* ********** */

    // Handle body
    std::string content_length = getHeader("Content-Length");
    int contentLength = atoi(content_length.c_str());


    std::string content_type = getHeader("Content-Type");
    std::string initialBodyData = request.substr(headerEnd + 4);

    if (contentLength > CLIENT_MAX_BODY_SIZE) {
        throw ParsingErrorException(CONTENT_LENGTH, "content length is above limit");
	}
	if (content_type.find("multipart/form-data") != content_type.npos
		|| getHeader("content-disposition") != "") {
		/* Leave it for now, this should be checked */
		if (initialBodyData != "") {
			std::cout << "Initial Body is expected to be empty" << std::endl;
			throw ParsingErrorException(INTERRUPT, "unexpected request");
		}
		/* ************** */
		_readBodyFile(contentLength);
	} else if (!headersComplete) {
		_readBodyStream();
	} else if (getHeader("Transfer-Encoding") != "chunked") {
        _readBody(contentLength, initialBodyData); // works incorect with some types of data in body
    } else {
        /* Chunked data recieved or with no Content-lenght */
        _readBodyChunked(initialBodyData);
    }
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
        key = utils::toLowerCase(key);
        _headers[key] = value;
    }
}

void Request::_readBody(int contentLength, const std::string& initialData) {
    _body = initialData;
    int remainingBytes = contentLength - initialData.length();

    if (remainingBytes > 0) {
        char *buffer = new char[remainingBytes + 1];
        int bytesRead = recv(_clientSocket, buffer, remainingBytes, 0);
        std::cout << "Additional body bytes read: " << bytesRead << std::endl;
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            _body += buffer;
        }
        delete[] buffer;
    }

    std::cout << "Total body length: " << _body.length() << std::endl;
}

void Request::_readBodyChunked(const std::string& initialData) {
    int bytesRead;
    char buffer[BUFFER_SIZE + 1];

    _body += initialData;
    bzero(buffer, BUFFER_SIZE + 1);
    bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE, 0);
	if (bytesRead < 0)
		throw ParsingErrorException(INTERRUPT, "bad socket");
    while (bytesRead > 0) {
        _body += buffer;
        bzero(buffer, BUFFER_SIZE + 1);
        bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == 0) {
            throw ParsingErrorException(INTERRUPT, "unexpected connection interrupt");
        } else if (bytesRead < 0) {
			throw ParsingErrorException(INTERRUPT, "bad socket");
        }
        std::cout << "Additional body bytes read: " << bytesRead << std::endl;
        std::cout << _body.size() << std::endl;
        if (bytesRead > CLIENT_MAX_BODY_SIZE)
            throw ParsingErrorException(CONTENT_LENGTH, "body is too big");
    }
    std::cout << "Total body length: " << _body.length() << std::endl;
}

void Request::_readBodyFile(int content_length)
{
	int bytesRead;
    char buffer[BUFFER_SIZE];
	char *tmp;
	std::string stream;

	(void)content_length;

	std::string boundary = getHeader("Content-Type");
	int pos = boundary.find("boundary=");
	boundary = boundary.substr(pos + 9);

	bzero(buffer, BUFFER_SIZE);
	bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE, 0);
	std::cout << CYAN << buffer << RESET << std::endl;
	tmp = strstr(buffer, boundary.c_str());
	if (tmp) {
		/* It indicaes to the begining of the file
		* and/or end of file */
		if (tmp - buffer == 2) {
			/* File is in the beggining of the body
			* start process as new file*/
			tmp += boundary.length() + 2;
			/* Start parsing file header */


			/***********/
			tmp = strstr(tmp, "\r\n\r\n") + 4;
			std::cout << GREEN << tmp << RESET << std::endl;
		} else {
			/* Request contain 2 files or parts of 2(or more) files*/
			/* Do we handle it? */
		}
	}
	tmp = strstr(buffer, "\r\n\r\n");
	if (tmp) {
		/* There is file header with details */
	}

	/* Skip header */

	std::string	file_name = utils::chunkFileName(getSocket());
	if (!access(file_name.c_str(), W_OK | R_OK))
		std::cout << BLACK << "File exists with permissions" << RESET << std::endl; // file exists, but it is from previous request?
	int file_fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0600);
	if (!file_fd)
		throw ParsingErrorException(FILE_SYSTEM, strerror(errno));

	while (bytesRead > 0)
	{
		bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE, 0);
		std::cout << "Btyes read: " << bytesRead << std::endl;
		if (bytesRead < 0)
			break ;
		write(file_fd, buffer, bytesRead);
		bzero(buffer, BUFFER_SIZE);
	}
	close(file_fd);
}

void Request::_readBodyStream()
{
	int bytesRead = 1;
	char buffer[BUFFER_SIZE];

	bzero(buffer, BUFFER_SIZE);
	std::string	file_name = utils::chunkFileName(getSocket());
	if (!access(file_name.c_str(), W_OK | R_OK))
		std::cout << BLACK << "File exists with permissions" << RESET << std::endl; // file exists, but it is from previous request?
	int file_fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0600);
	while (bytesRead > 1)
	{
		bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE, 0);
		if (bytesRead < 0)
			throw ParsingErrorException(INTERRUPT, "bad socket");
		write(file_fd, buffer, bytesRead);
		bzero(buffer, BUFFER_SIZE);
	}
	close(file_fd);
}

void Request::addHeader(const std::string& key, const std::string& value) {
    std::string lowercase_key;

    lowercase_key = key;
    lowercase_key = utils::toLowerCase(lowercase_key);
    _headers[lowercase_key] = value;
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
    std::string lowercase_key;

    lowercase_key = key;
    lowercase_key = utils::toLowerCase(lowercase_key);
    std::map<std::string, std::string>::const_iterator it = _headers.find(lowercase_key);
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

int Request::getSocket() const
{
    return _clientSocket;
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
    os << "HTTP Request Details:" << std::endl;
    os << "Socket file descriptor: " << request._clientSocket << std::endl;
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
	this->type = type;
    bzero(_error, 256);
    strncpy(_error, "Request parsing error: ", 24);
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Request::ParsingErrorException::what() const throw() {
	return _error;
}

Request::SocketCloseException::SocketCloseException(const char *error_msg) {
    strncpy(_error, "Socket closed: ", 27);
    strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Request::SocketCloseException::what() const throw() {
    return _error;
}
