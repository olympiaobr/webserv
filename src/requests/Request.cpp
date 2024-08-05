#include "Request.hpp"
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

Request::Request(int clientSocket, ServerConfig &config)
	: _clientSocket(clientSocket), _config(config) {}

Request::~Request() {}

void Request::parse() {
    char buffer[BUFFER_SIZE];
    std::string request;
    ssize_t bytesRead;
    bool headersComplete = false;
    size_t headerEnd;

    // Set socket to non-blocking mode


    while (true) {
        // Read from socket until the end of the headers is found
        bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
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
			throw ParsingErrorException(BAD_REQUEST, "malformed request");
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
	// std::map<std::string, std::string> header = 	getHeaders();
	// std::cout << MAGENTA << "Headers: " << std::endl;
	// for (std::map<std::string, std::string>::const_iterator it=header.begin(); it != header.end(); ++it) {

	// 	std::cout << it->first << ": " << it->second << std::endl;
	// }
	// std::cout << RESET;
	/* ********** */

    // Handle body
    std::string content_length = getHeader("Content-Length");
    int contentLength = atoi(content_length.c_str());


    std::string content_type = getHeader("Content-Type");
    std::string initialBodyData = request.substr(headerEnd + 4);

    if (contentLength > CLIENT_MAX_BODY_SIZE) {
        throw ParsingErrorException(CONTENT_LENGTH, "content length is above limit");
	}
	const char *body_buffer = utils::strstr(buffer, "\r\n\r\n", BUFFER_SIZE - 1);
	body_buffer += 4;
	bytesRead -= body_buffer - buffer;
	if (*body_buffer == 0) {
		bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
		if (!bytesRead)
			throw ParsingErrorException(INTERRUPT, "form-data is empty");
		body_buffer = buffer;
	}
	if (content_type.find("multipart/form-data") != content_type.npos) {
		/* Leave it for now, this should be checked */
		/* ************** */
		_readBodyFile(body_buffer, bytesRead);
    } else if (getHeader("Transfer-Encoding") == "chunked"){
        /* Chunked data recieved or with no Content-lenght */
        _readBodyChunked(body_buffer, bytesRead);
    } else {
        _readBody(contentLength, initialBodyData); // works incorect with some types of data in body
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
        int bytesRead = recv(_clientSocket, buffer, remainingBytes, MSG_DONTWAIT);
        std::cout << "Additional body bytes read: " << bytesRead << std::endl;
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            _body += buffer;
        }
        delete[] buffer;
    }

    std::cout << "Total body length: " << _body.length() << std::endl;
}

void Request::_readBodyChunked(const char *buffer, ssize_t bytesRead) {
	int read_len;
	char *stream;

	stream = (char *)buffer;
	std::string	file_name = utils::chunkFileName(getSocket());
	if (!access(file_name.c_str(), W_OK | R_OK))
		std::cout << BLACK << "File exists with permissions" << RESET << std::endl; // file exists, but it is from previous request?
	int file_fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0600);
	if (file_fd < 0)
		throw ParsingErrorException(FILE_SYSTEM, "chunk temp file open failed");
	while (*stream) {
		read_len = atoi(stream);
		if (read_len == 0) {
			close(file_fd);
			utils::saveFile(file_name, _config);
			return ;
		}
		stream = utils::strstr(stream, "\r\n", bytesRead) + 2;
		bytesRead -= stream - buffer;
		buffer = stream;
		write(file_fd, stream, read_len);
		stream = utils::strstr(stream, "\r\n", bytesRead) + 2;
		bytesRead -= stream - buffer;
	}
	close(file_fd);
}

void Request::_readBodyFile(const char *buffer, ssize_t bytesRead)
{
	char *start_pos;
	std::string stream;

	std::string boundary = getHeader("Content-Type");
	int pos = boundary.find("boundary=");
	boundary = boundary.substr(pos + 9);
	boundary = "--" + boundary;

	start_pos = utils::strstr(buffer, (char *)boundary.c_str(), bytesRead);
	if (start_pos) {
		/* Find start of data */
		start_pos += boundary.length() + 2;

		/* Start parsing data header */
		std::string unique_filename;
		{
			const char *header_pos = start_pos;
			start_pos = utils::strstr(start_pos, (const char *)"\r\n\r\n", bytesRead - (start_pos - buffer)) + 4;
			std::string headers = header_pos;
			headers = headers.substr(0, start_pos - header_pos);
			/* ************************* */

			/* Does it alway have filename in header? */
			std::string new_file_name = headers.substr(headers.find("filename=") + 9);
			new_file_name = new_file_name.substr(0, new_file_name.find('\r'));
			new_file_name.erase(new_file_name.begin());
			new_file_name.erase(new_file_name.end() - 1);
			int i = 1;

			unique_filename = _config.root + "upload" + new_file_name;
			while (access(unique_filename.c_str(), F_OK) == 0) {
				std::stringstream ss;
				ss << i;
				std::string value;
				ss >> value;
				unique_filename = new_file_name + " (" + value + ")";
				i++;
			}
		}
		int file_fd = open(unique_filename.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0600);
		if (!file_fd)
			throw ParsingErrorException(FILE_SYSTEM, strerror(errno));
		/* ******************************************** */

		/* Skip header and take data body length */
		size_t len = bytesRead - (start_pos - buffer);
		std::string boundary_end = boundary + "--";
		/* Checking boundaries */
		{
			const char *boundary_pos = utils::strstr(start_pos, boundary.c_str(), len);
			const char *boundary_end_pos = utils::strstr(start_pos, boundary_end.c_str(), len);
			/* Check if this chunk contains another data */
			if (boundary_pos && boundary_pos != boundary_end_pos) {
				_readBodyFile(boundary_pos, len - (boundary_pos - start_pos));
				len -= bytesRead - (boundary_pos - buffer) + 2;
			}
			/* Check if this chunk contains EOF */
			else if (boundary_end_pos) {
				len -= bytesRead - (boundary_end_pos - buffer) + 2;
			}
		}
		/* Write data to file */
		write(file_fd, start_pos, len);

		/* Repeat */
		// while (bytesRead > 0) {
		// 	bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE, MSG_DONTWAIT);
		// 	if (bytesRead < 0)
		// 		break;
		// 	total_read += bytesRead;
		// 	start_pos = utils::strstr(buffer, boundary_end.c_str(), bytesRead);
		// 	if (start_pos) {
		// 		std::cout << RED << *(start_pos - 1) << std::endl;
		// 		write(file_fd, buffer, start_pos - buffer);
		// 		break;
		// 	}
		// 	else
		// 		write(file_fd, buffer, bytesRead);
		// }
		close (file_fd);
	}
}

// void Request::_readBodyStream()
// {
// 	int bytesRead = 1;
// 	char buffer[BUFFER_SIZE];

// 	bzero(buffer, BUFFER_SIZE);
// 	std::string	file_name = utils::chunkFileName(getSocket());
// 	if (!access(file_name.c_str(), W_OK | R_OK))
// 		std::cout << BLACK << "File exists with permissions" << RESET << std::endl; // file exists, but it is from previous request?
// 	int file_fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0600);
// 	while (bytesRead > 1)
// 	{
// 		bytesRead = recv(_clientSocket, buffer, BUFFER_SIZE, 0);
// 		if (bytesRead < 0)
// 			throw ParsingErrorException(INTERRUPT, "bad socket");
// 		write(file_fd, buffer, bytesRead);
// 		bzero(buffer, BUFFER_SIZE);
// 	}
// 	close(file_fd);
// }

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
