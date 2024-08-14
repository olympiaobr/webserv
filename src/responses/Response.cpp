#include "Response.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <cstdio>

Response::Response(const ServerConfig& config, char* buffer, int buffer_size):
	_config(config), _statusCode(-1), _buffer(buffer), _buffer_size(buffer_size), _content_length(0) {}

Response::Response(const ServerConfig& config, int errorCode, char* buffer, int buffer_size)
	: _httpVersion("HTTP/1.1"), _config(config), _buffer(buffer), _buffer_size(buffer_size), _content_length(0) {
	initializeHttpErrors();
	_setError(errorCode);
}

Response::Response(const Request& req, const ServerConfig& config, char* buffer, int buffer_size)
    : _httpVersion("HTTP/1.1"), _config(config), _buffer(buffer), _buffer_size(buffer_size), _content_length(0) {
    initializeHttpErrors();

    // Ensure the request is using HTTP/1.1 standard
    if (req.getHttpVersion() != "HTTP/1.1") {
        _setError(505);
        return;
    }

    // Ensure the hostname is valid
    if (std::find(
            config.hostnames.begin(),
            config.hostnames.end(),
            req.getHost()
        ) == config.hostnames.end()) {
        _setError(400);
        return;
    }

    // Ensure the method is allowed in the requested route
    const RouteConfig* routeConfig = _findMostSpecificRouteConfig(req.getUri());
    if (!routeConfig) {
        _setError(404);
        return;
    }
    if (std::find(routeConfig->allowed_methods.begin(), routeConfig->allowed_methods.end(), req.getMethod()) == routeConfig->allowed_methods.end()) {
        _setError(405);
        return;
    }
    _dispatchMethodHandler(req);
}


const RouteConfig* Response::_findMostSpecificRouteConfig(const std::string& uri) const {
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

void Response::_dispatchMethodHandler(const Request& req) {
    if (req.getMethod() == "GET")
        _handleGetRequest(req);
    else if (req.getMethod() == "POST")
        _handlePostRequest(req);
    else if (req.getMethod() == "DELETE")
        _handleDeleteRequest(req);
    else
        _setError(405);
}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		_httpVersion = other._httpVersion;
		_statusCode = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers = other._headers;
		_content = other._content;
		_buffer = other._buffer;
		_buffer_size = other._buffer_size;
		_httpErrors = other._httpErrors;
		_content_length = other._content_length;
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

void Response::_handleGetRequest(const Request& req) {
    std::string path = _config.root + req.getUri();

    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == 0) {
        if (S_ISDIR(fileStat.st_mode)) {  // Check if it's a directory
            std::string indexPath = path + "/index.html";
            if (stat(indexPath.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode)) {
                // std::string content = _readFile(indexPath);
				try {
					setStatus(200);
					addHeader("Content-Type", _getMimeType(indexPath));
					generateResponse(indexPath);
				} catch (Response::FileSystemErrorException &e) {
                    _setError(404);
				} catch (Response::ContentLengthException &e) {
					_setError(413);
				}
            } else {
                const RouteConfig* routeConfig = _findMostSpecificRouteConfig(req.getUri());
                if (routeConfig && routeConfig->autoindex) {
					try {
						setStatus(200);
						addHeader("Content-Type", "text/html");
						generateDirectoryListing(path);
					} catch (Response::FileSystemErrorException &e) {
						_setError(404);
					} catch (Response::ContentLengthException &e) {
						_setError(413);
					}
                } else {
                    _setError(404); // No index.html and autoindex is not enabled
                }
            }
            return;
        }
        // It's not a directory, handle as a regular file
        // std::string content = _readFile(path);
		try {
            setStatus(200);
            addHeader("Content-Type", _getMimeType(path));
			addHeader("Content-Disposition", "inline");
			generateResponse(path);
		} catch (Response::FileSystemErrorException &e) {
			_setError(404);
		} catch (Response::ContentLengthException &e) {
			_setError(413);
		}
    } else {
        _setError(404); // File or directory not found
    }
}

/* send page ? */
void Response::_handlePostRequest(const Request& req) {
    setStatus(200);
    addHeader("Content-Type", "text/html");
    std::string directoryPath = _config.root + req.getUri();
    {
        std::ostringstream listing;
        DIR* dir = opendir(directoryPath.c_str());
        if (dir == NULL) {
            throw FileSystemErrorException("cannot open directory");
        }
        struct dirent* entry;
        std::string stylesPath = _config.root + "/css/styles.css";
        std::ifstream styles(stylesPath.c_str());
        if (!styles)
            throw FileSystemErrorException("cannot open styles.css");
        listing << "<html><head><title> File uploaded! " << directoryPath
                << "</title>" << styles.rdbuf() << "</head><body><h1>File uploaded!</h2><h2>Index of " << directoryPath
                << "</h2><ul>";
        styles.close();
        while ((entry = readdir(dir)) != NULL) {
            listing << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>";
        }
        closedir(dir);
        listing << "</ul></body></html>";
        std::memset(_buffer, 0, _buffer_size);
        std::string list = listing.str();
        addHeader("Content-Length", utils::to_string(list.size()));
        std::string headers = _headersToString();
        if (list.size() + headers.size() > _buffer_size)
            throw ContentLengthException("body is too long");
        std::memcpy(_buffer, headers.c_str(), headers.size());
        char *body = _buffer + headers.size();
        std::memcpy(body, list.c_str(), list.size());
        _content = _buffer;
        _content_length = headers.size() + list.size();
    }
}

void Response::_handleDeleteRequest(const Request& req)
{
    std::string uri = req.getUri();
    std::string filePath = _config.root + uri;

    // Check if the file exists
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) != 0) {
        _setError(404);
        return;
    }
	try
	{
        setStatus(200);
		addHeader("Content-Type", _getMimeType(filePath));
		generateResponse(filePath);
	} catch (Response::FileSystemErrorException &e) {
        _setError(500);
	} catch (Response::ContentLengthException &e) {
		_setError(413);
	}
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

int Response::getStatusCode() {
	return _statusCode;
}

/* We need plan B if original function won't work */
void Response::_setError(int code) {
	std::string path;
	std::map<int, std::string>::const_iterator it = _config.error_pages.find(code);

	if (it != _config.error_pages.end())
		path = it->second;
	else
		throw std::logic_error("Error code not found in configuration");
	std::string filename = _config.root + path;

	try
	{
        setStatus(code);
        addHeader("Content-Type", _getMimeType(filename));
		generateResponse(filename);
	} catch (Response::FileSystemErrorException &e) {
		_setError(404);
	} catch (Response::ContentLengthException &e) {
		_setError(413);
	}
}

void Response::addHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

std::string Response::_headersToString() const {
    std::ostringstream oss;
    oss << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    oss << "\r\n";
    return oss.str();
}

void Response::generateResponse(const std::string& filename) {
    std::memset(_buffer, 0, _buffer_size);
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        throw FileSystemErrorException("could not open the file");
    }
    std::string headers = _headersToString();
    size_t headerSize = headers.size();
    if (headerSize + 50 > _buffer_size) {
        close(fd);
        throw ContentLengthException("Headers are too large for buffer");
    }
    std::memmove(_buffer, headers.c_str(), headerSize);
    char* body = _buffer + headerSize;
    ssize_t maxBodySize = _buffer_size - headerSize - 50;
    ssize_t bytesRead = read(fd, body, maxBodySize);
    if (bytesRead < 0) {
        close(fd);
        throw FileSystemErrorException("could not read the file");
    }
    if (bytesRead == maxBodySize) {
        close(fd);
        throw ContentLengthException("Headers are too large for buffer");
    }
    _content_length = headerSize + bytesRead;
    addHeader("Content-Length", _toString(bytesRead));
    headers = _headersToString();
    size_t newHeaderSize = headers.size();
    if (newHeaderSize > headerSize) {
        char* newBodyStart = _buffer + newHeaderSize;
        std::memmove(newBodyStart, body, bytesRead);
        _content_length = newHeaderSize + bytesRead;
    }
    std::memmove(_buffer, headers.c_str(), newHeaderSize);
    _content = _buffer;
    _content_length = newHeaderSize + bytesRead;
    close(fd);
}


void Response::generateDirectoryListing(const std::string& directoryPath) {
    std::ostringstream listing;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir == NULL) {
        throw FileSystemErrorException("cannot open directory");
    }

    struct dirent* entry;
    std::string stylesPath = _config.root + "/css/styles.css";
    std::ifstream styles(stylesPath.c_str());
    if (!styles) {
        throw FileSystemErrorException("cannot open styles.css");
    }

    listing << "<html><head><title> Directory navigation " << directoryPath
            << "</title>" << styles.rdbuf() << "</head><body><h2>Index of " << directoryPath
            << "</h2><ul>";
    styles.close();

    while ((entry = readdir(dir)) != NULL) {
        listing << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>";
    }
    closedir(dir);
    listing << "</ul></body></html>";

    std::memset(_buffer, 0, _buffer_size);
    std::string list = listing.str();
    addHeader("Content-Length", utils::to_string(list.size()));
    std::string headers = _headersToString();
    if (list.size() + headers.size() > _buffer_size) {
        throw ContentLengthException("body is too long");
    }
    std::memcpy(_buffer, headers.c_str(), headers.size());
    char *body = _buffer + headers.size();
    std::memcpy(body, list.c_str(), list.size());
    _content = _buffer;
    _content_length = headers.size() + list.size();
}


void Response::generateCGIResponse(const std::string &cgi_response)
{
	// addHeader("Content-Type", "??");
	std::string cgi_body = cgi_response.substr(cgi_response.find("\r\n\r\n") + 4);
	addHeader("Content-Length", _toString(cgi_body.length()));
	std::string headers = _headersToString();
	char* body;
	memset(_buffer, 0, _buffer_size);
	memmove(_buffer, headers.c_str(), headers.size());
	body = _buffer + headers.size() - 4;
	memmove(body, cgi_response.c_str(), cgi_response.size());
	_content_length = headers.size() - 4 + cgi_response.size();
	_content = _buffer;
}

const char *Response::getContent()
{
	return _content;
}

ssize_t Response::getContentLength()
{
	return _content_length;
}

std::string Response::_getMimeType(const std::string& filename) {
    if (filename.find(".html") != std::string::npos) return "text/html";
    if (filename.find(".css") != std::string::npos) return "text/css";
    if (filename.find(".js") != std::string::npos) return "application/javascript";
    if (filename.find(".ico") != std::string::npos) return "image/x-icon";
	if (filename.find(".pdf") != std::string::npos) return "application/pdf";
	if (filename.find(".png") != std::string::npos) return "image/png";
	if (filename.find(".jpeg") != std::string::npos) return "image/jpeg";
	if (filename.find(".jpg") != std::string::npos) return "image/jpg";
	if (filename.find(".gif") != std::string::npos) return "image/gif";
	if (filename.find(".webp") != std::string::npos) return "image/webp";
	if (filename.find(".svg") != std::string::npos) return "image/svg+xml";
	if (filename.find(".mpeg") != std::string::npos) return "audio/mpeg";
	if (filename.find(".wav") != std::string::npos) return "audio/wav";
	if (filename.find(".mp4") != std::string::npos) return "video/mp4";
	if (filename.find(".webm") != std::string::npos) return "video/webm";
	if (filename.find(".mov") != std::string::npos) return "video/mov";
	if (filename.find(".json") != std::string::npos) return "application/json";
	if (filename.find(".xml") != std::string::npos) return "application/xml";
	if (filename.find(".zip") != std::string::npos) return "application/zip";
	if (filename.find(".file") != std::string::npos) return "application/octet-stream";
    return "text/plain";
}

std::string Response::_toString(size_t num) const {
    std::ostringstream oss;
    oss << num;
    return oss.str();
}

Response::FileSystemErrorException::FileSystemErrorException(const char *error_msg) {
    std::memset(_error, 0, 256);
    strncpy(_error, "Response file system error: ", 29);
    _error[27] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Response::FileSystemErrorException::what() const throw() {
	return _error;
}

Response::ContentLengthException::ContentLengthException(const char *error_msg) {
    std::memset(_error, 0, 256);
    strncpy(_error, "Content length is too long: ", 29);
    _error[27] = '\0';
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Response::ContentLengthException::what() const throw() {
	return _error;
}
