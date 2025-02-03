#include "Response.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <cstdio>
#include <dirent.h>

Response::Response()
{
    _httpVersion = "HTTP/1.1";
    _statusCode = 0;
    initializeHttpErrors();
    _buffer_size = RESPONSE_MAX_BODY_SIZE;
    _buffer = new char[_buffer_size];
}

Response::~Response()
{
    delete[] _buffer;
}

// Response::Response(ServerConfig *config, int buffer_size) : _config(config), _statusCode(-1), _buffer_size(buffer_size), _content_length(0)
// {
//     _statusCode = 0;
//     initializeHttpErrors();
//     _buffer = new char[_buffer_size];
// }

// Response::Response(ServerConfig* config, int errorCode, int buffer_size)
// 	: _httpVersion("HTTP/1.1"), _config(config), _buffer_size(buffer_size), _content_length(0) {
//     _statusCode = 0;
//     initializeHttpErrors();
// 	setError(errorCode);
//     _buffer = new char[_buffer_size];
// }

// Response::Response(const Request& req, ServerConfig* config, int buffer_size)
//     : _httpVersion("HTTP/1.1"), _config(config), _buffer_size(buffer_size), _content_length(0) {

//     _buffer = new char[_buffer_size];

//     initializeHttpErrors();

//     if (req.getHttpVersion() != "HTTP/1.1") {
//         setError(505);
//         return;
//     }
//     if (std::find(
//             config->hostnames.begin(),
//             config->hostnames.end(),
//             req.getHost()
//         ) == config->hostnames.end()) {
//         setError(400);
//         return;
//     }
//     // const RouteConfig* route_config = _findMostSpecificRouteConfig(req.getUri());
//     const RouteConfig* route_config = req.getRouteConfig();
//     if (!route_config) {
//         setError(404);
//         return;
//     }
// 	if (this->_handleRedir(route_config->redirect_status_code, route_config->redirect_url)) {
//         return;
//     }
//     if (std::find(route_config->allowed_methods.begin(), route_config->allowed_methods.end(), req.getMethod()) == route_config->allowed_methods.end()) {
//         setError(405);
//         return;
//     }
//     _dispatchMethodHandler(req, route_config);
// }

bool Response::_handleRedir(int redirect_status_code, const std::string& redirect_url) {

    if (redirect_status_code == 0 || redirect_url.empty()) {

        return false;

    }
    std::cout << "Redirecting to: " << redirect_url << " with status code: " << redirect_status_code << std::endl;

    setStatus(redirect_status_code);
    addHeader("Location", redirect_url);
    addHeader("Content-Length", "0");
    _connection = "close";

    std::string headers = _headersToString();
    if (headers.size() > _buffer_size) {

        throw std::runtime_error("Buffer overflow: headers too large for buffer.");

    }
    std::memset(_buffer, 0, _buffer_size);
    std::memcpy(_buffer, headers.c_str(), headers.size());
    _content = _buffer;
    _content_length = headers.size();
    return true;
}

void Response::_dispatchMethodHandler(const Request& req, const RouteConfig* route_config) {
	try {
		if (req.getMethod() == "GET" || req.getMethod() == "HEAD")
			_handleGetRequest(req, route_config);
		else if (req.getMethod() == "POST" || req.getMethod() == "PUT")
			_handlePostRequest(req, route_config);
		else if (req.getMethod() == "DELETE")
			_handleDeleteRequest(req, route_config);
		else
			setError(405);
	} catch (Response::FileSystemErrorException &e) {
		setError(404);
	} catch (Response::ContentLengthException &e) {
		setError(413);
	}
}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		_httpVersion = other._httpVersion;
		_statusCode = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers = other._headers;
		_content = other._content;
        _buffer_size = other._buffer_size;
        delete[] _buffer;
		_buffer = new char[other._buffer_size];
        std::copy(other._buffer, other._buffer + _buffer_size, _buffer);
        _httpErrors = other._httpErrors;
		_content_length = other._content_length;
        _headers_length = other._headers_length;
        _connection = other._connection;
    }
	return *this;
}

void Response::setConfig(ServerConfig &config) {
    _config = &config;
}

void Response::initializeHttpErrors() {
    _httpErrors[200] = "OK";
    _httpErrors[201] = "Created";
    // _httpErrors[300] = "Multiple Choices";
    _httpErrors[307] = "Temporary Redirect";
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

void Response::_handleGetRequest(const Request& req, const RouteConfig* route_config) {
    std::string uri = req.getUri();
    std::string path = route_config->root;

    // std::cout << "Original path: " << path << std::endl;
    path += uri;
    // std::cout << "Requested path: " << path << std::endl;
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == 0) {
        if (S_ISDIR(fileStat.st_mode)) {
            if (path[path.length() - 1] != '/') {
                path += "/";
            }
            std::string indexPath = path + route_config->default_file;
            // std::cout << "Looking for index file: " << indexPath << std::endl;
            if (stat(indexPath.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode)) {
                // std::cout << "Index file found: " << indexPath << std::endl;
                setStatus(200);
                addHeader("Content-Type", _getMimeType(indexPath));
                addHeader("Set-Cookie", req.getSession());
                generateResponse(indexPath);
            } else if (route_config->autoindex) {
                // std::cout << "Autoindex is enabled, generating directory listing." << std::endl;
                setStatus(200);
                addHeader("Content-Type", "text/html");
                addHeader("Set-Cookie", req.getSession());
                generateDirectoryListing(path);
            } else {
                // std::cout << "No index file found, and autoindex is off, returning 404." << std::endl;
                setError(404);
            }
        } else {
            // std::cout << "Serving regular file: " << path << std::endl;
            setStatus(200);
            addHeader("Content-Type", _getMimeType(path));
            addHeader("Set-Cookie", req.getSession());
            generateResponse(path);
        }
        if (req.getMethod() == "HEAD") {
            _content_length = _headers_length;
        }
    }
    else {
        // std::cout << "File or directory not found: " << path << std::endl;
        setError(404);

        if (req.getMethod() == "HEAD") {
            _content_length = _headers_length;
        }
    }
}

/* send page ? */
void Response::_handlePostRequest(const Request& req, const RouteConfig* route_config) {
    setStatus(200);
    addHeader("Content-Type", "text/html");
	std::string directoryPath = _config->root + req.getUri();
	{
		std::ostringstream listing;
		DIR* dir = opendir(directoryPath.c_str());
		if (dir == NULL) {
			throw FileSystemErrorException("cannot open directory");
		}

		struct dirent* entry;
		std::string file = _config->root + "/css/styles.css";
		std::ifstream styles(file.c_str());
		if (!styles)
			throw FileSystemErrorException("cannot open directory");
		listing << "<html><head><title> File uploaded! " << directoryPath
				<< "</title> <link rel=\"stylesheet\" href=\"" << styles.rdbuf()
				<< "\">" << "</head><body><h1>File uploaded!</h2><h2>Index of " << directoryPath
				<< "</h2><ul>";
		styles.close();
		while ((entry = readdir(dir)) != NULL) {
			listing << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>";
		}
		closedir(dir);
		listing << "</ul></body></html>";
		std::memset(_buffer, 0, _buffer_size);
		std::string list = listing.str();
		addHeader("Content-Length", utils::toString(list.size()));
		std::string headers = _headersToString();
		if (list.size() + headers.size() > _buffer_size)
			throw ContentLengthException("body is too long");
		std::memcpy(_buffer, headers.c_str(), headers.size());
		char *body = _buffer + headers.size();
		std::memcpy(body, list.c_str(), list.size());
		_content = _buffer;
		_content_length = headers.size() + list.size();
	}
	(void)route_config;
}

void Response::_handleDeleteRequest(const Request& req, const RouteConfig* route_config)
{
    std::string uri = req.getUri();
    std::string filePath = _config->root + uri;

    // Check if the file exists
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) != 0) {
        setError(404);
        return;
    }
	setStatus(200);
	addHeader("Content-Type", _getMimeType(filePath));
	addHeader("Set-Cookie", req.getSession());
    if (!utils::fileExists(filePath)) {
        throw FileSystemErrorException("File does not exist");
    }
    else if (!utils::deleteFile(filePath)) {
        throw FileSystemErrorException("Fobidden");
    }
	(void)route_config;
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
void Response::setError(int code) {
	std::string path;
	std::map<int, std::string>::const_iterator it = _config->error_pages.find(code);

	if (it != _config->error_pages.end())
		path = it->second;
	else {
        std::cerr << code << std::endl;
		throw std::logic_error("Error code not found in configuration");
    }
	std::string filename = _config->root + path;

	try
	{
        setStatus(code);
        addHeader("Content-Type", _getMimeType(filename));
		generateResponse(filename);
	} catch (Response::FileSystemErrorException &e) {
		setError(404);
	} catch (Response::ContentLengthException &e) {
		setError(413);
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
    _headers_length = newHeaderSize;
    close(fd);
}

void Response::generateDirectoryListing(const std::string& directoryPath) {
    std::ostringstream listing;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir == NULL) {
        throw FileSystemErrorException("cannot open directory");
    }

	struct dirent* entry;
	std::string file = _config->root + "/css/styles.css";
	std::ifstream styles(file.c_str());
	if (!styles)
		throw FileSystemErrorException("cannot open directory");
	listing << "<html><head><title> Directory navigation " << directoryPath
			<< "</title> <link rel=\"stylesheet\" href=\"" << styles.rdbuf()
			<< "\">" << "</head><body><h2>Index of " << directoryPath
			<< "</h2><ul>";
	styles.close();
	while ((entry = readdir(dir)) != NULL) {
		listing << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>";
	}
	closedir(dir);
	listing << "</ul></body></html>";
	std::memset(_buffer, 0, _buffer_size);
	std::string list = listing.str();
	addHeader("Content-Length", utils::toString(list.size()));
	std::string headers = _headersToString();
	if (list.size() + headers.size() > _buffer_size)
		throw ContentLengthException("body is too long");
	std::memcpy(_buffer, headers.c_str(), headers.size());
	char *body = _buffer + headers.size();
	std::memcpy(body, list.c_str(), list.size());
	_content = _buffer;
	_content_length = headers.size() + list.size();
    _headers_length = headers.size();
}


void Response::generateCGIResponse(const std::string &cgi_response)
{
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

ServerConfig *Response::getConfig()
{
    return _config;
    // TODO: insert return statement here
}

void Response::setContent(ssize_t move)
{
    _content = _content + move;
    _content_length -= move;
}

void Response::initialize(const Request &req)
{
    if (req.getHttpVersion() != "HTTP/1.1")
    {
        setError(505);
        return;
    }
    if (std::find(
            _config->hostnames.begin(),
            _config->hostnames.end(),
            req.getHost()) == _config->hostnames.end())
    {
        setError(400);
        return;
    }
    // const RouteConfig* route_config = _findMostSpecificRouteConfig(req.getUri());
    const RouteConfig *route_config = req.getRouteConfig();
    if (!route_config)
    {
        setError(404);
        return;
    }
    if (this->_handleRedir(route_config->redirect_status_code, route_config->redirect_url))
    {
        return;
    }
    if (std::find(route_config->allowed_methods.begin(), route_config->allowed_methods.end(), req.getMethod()) == route_config->allowed_methods.end())
    {
        setError(405);
        return;
    }
    _dispatchMethodHandler(req, route_config);
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
