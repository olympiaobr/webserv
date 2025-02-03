#include "Request.hpp"
#include "../server/Server.hpp"

// Request::Request(int clientSocket, ServerConfig *config, int buffer_len)
// 	: buffer_length(buffer_len), _clientSocket(clientSocket), _config(config)
// {
// 	buffer = new char[buffer_length];
// 	total_read = 0;
// }

Request::Request(const Request &src, size_t extend)
{
	_session_id = src._session_id;
	_clientSocket = src._clientSocket;
	_method = src._method;
	_uri = src._uri;
	_httpVersion = src._httpVersion;
	_headers = src._headers;
	_body = src._body;
	_config = src._config;
	_route_config = src._route_config;
	buffer_length = src.buffer_length + extend;
	buffer = new char[buffer_length];
	std::copy(src.buffer, src.buffer + src.buffer_length, buffer);
}

Request::Request(): buffer_length(2048) {
	total_read = 0;
	buffer = new char[buffer_length];
}

Request::Request(int clientSocket) : buffer_length(2048)
{
	total_read = 0;
	buffer = new char[buffer_length];
	_clientSocket = clientSocket;
}

Request::~Request() {
	if (buffer)
		delete[] buffer;
}

Request &Request::operator=(const Request &src)
{
	_session_id = src._session_id;
	_clientSocket = src._clientSocket;
	_method = src._method;
	_uri = src._uri;
	_httpVersion = src._httpVersion;
	_headers = src._headers;
	_body = src._body;
	_config = src._config;
	_route_config = src._route_config;
	buffer_length = src.buffer_length;
	delete[] buffer;
	buffer = new char[buffer_length];
	std::copy(src.buffer, src.buffer + buffer_length, buffer);
	return *this;
}

void Request::parseHeaders() {
    std::string request;
    size_t headerEnd;

    buffer[total_read] = 0;
    request += buffer;

    if (request.size() > buffer_length) {
        throw ParsingErrorException(BAD_REQUEST, "Header size exceeds buffer limit");
    }

    headerEnd = request.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::istringstream requestStream(request.substr(0, headerEnd));
        std::string line;
        if (std::getline(requestStream, line)) {
            _parseRequestLine(line);
        }
        while (std::getline(requestStream, line) && line != "\r") {
            _parseHeader(line);
        }
    } else {
        throw ParsingErrorException(BAD_REQUEST, "Malformed request");
    }

    size_t left_len = buffer_length - headerEnd - 4;
    total_read = left_len;
    char* body_part = buffer + headerEnd + 4;
    if (*body_part) {
        std::memmove(buffer, body_part, left_len);
    } else {
        std::memset(buffer, 0, buffer_length);
    }
}

RouteConfig* Request::_findMostSpecificRouteConfig(const std::string& uri)
{
    RouteConfig* bestMatch = NULL;
    size_t longestMatchLength = 0;
    for (std::map<std::string, RouteConfig>::iterator it = _config->routes.begin(); it != _config->routes.end(); ++it) {
        std::string basePath = it->first;
		if (*basePath.begin() == '~' && *(basePath.end() - 1) == '$')
		{
			basePath.erase(basePath.begin());
			basePath.erase(basePath.begin());
			basePath.erase(basePath.end() - 1);
			// std::cout << uri.rfind(basePath)  << std::endl;
			// std::cout << uri.length() - 1 << std::endl;
			if (uri.length() > 1 && uri.rfind(basePath) == uri.length() - basePath.length())
			{
				bestMatch = &it->second;
				longestMatchLength = 1; // >>??
				break;
			}		}
		else if (uri.find(basePath) == 0 && basePath.length() > longestMatchLength) {
            bestMatch = &it->second;
            longestMatchLength = basePath.length();
        }
    }
	if (_uri.length() > 1) {
    	_uri = _uri.substr(longestMatchLength - 1);
	}
    return bestMatch;
}

void Request::_parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    iss >> _method >> _uri >> _httpVersion;
	_uri = utils::decodePercentEncoding(_uri);
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
		if (key == "host") {
			value = value.substr(0, value.find(':'));
		}
        if (key == "cookie") {
            // Parse cookie header to extract session ID
            size_t sessionStart = value.find("session=");
            if (sessionStart != std::string::npos) {
                sessionStart += 8; // length of "session="
                size_t sessionEnd = value.find(';', sessionStart);
                if (sessionEnd == std::string::npos) {
                    sessionEnd = value.length();
                }
                _session_id = value.substr(sessionStart, sessionEnd - sessionStart);
            }
        }
        _headers[key] = value;
    }
}

void Request::readBodyChunked(char *buffer, ssize_t bytesRead) {
    char *stream = buffer;
    char *write_ptr = buffer;
    ssize_t remaining = bytesRead;
    size_t total_unchunked = 0;

    while (remaining > 0) {
        // Skip any leading CRLF
        while (remaining > 0 && (*stream == '\r' || *stream == '\n')) {
            stream++;
            remaining--;
        }

        if (remaining <= 0) break;

        // Find the end of chunk size line
        char *chunk_end = utils::strstr(stream, "\r\n", remaining);
        if (!chunk_end) {
            // Incomplete chunk header, wait for more data
            std::memmove(buffer, stream, remaining);
            total_read = remaining;
            return;
        }

        // Parse chunk size (in hex)
        char chunk_size_str[32] = {0};
        std::strncpy(chunk_size_str, stream, std::min(size_t(chunk_end - stream), size_t(31)));
        char *endptr;
        long chunk_size = std::strtol(chunk_size_str, &endptr, 16);

        if (endptr == chunk_size_str || chunk_size < 0) {
            throw ParsingErrorException(BAD_REQUEST, "invalid chunk size");
        }

        // Move past chunk size line
        stream = chunk_end + 2;
        remaining -= (chunk_end - buffer + 2);

        if (chunk_size == 0) {
            // Final chunk
            if (remaining >= 2 && stream[0] == '\r' && stream[1] == '\n') {
                // Valid end of chunked data
                *write_ptr = '\0';
                total_read = total_unchunked;
                return;
            }
            throw ParsingErrorException(BAD_REQUEST, "invalid chunked ending");
        }

        // Check if we have the full chunk data plus CRLF
        if (remaining < chunk_size + 2) {
            // Incomplete chunk, wait for more data
            std::memmove(buffer, stream - 2, remaining + 2); // Keep chunk header
            total_read = remaining + 2;
            return;
        }

        // Copy chunk data
        std::memmove(write_ptr, stream, chunk_size);
        write_ptr += chunk_size;
        total_unchunked += chunk_size;

        // Move past chunk data and its CRLF
        stream += chunk_size + 2;
        remaining -= (chunk_size + 2);
    }

    // If we get here, we need more data
    total_read = remaining;
    if (remaining > 0) {
        std::memmove(buffer, stream, remaining);
    }
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

bool Request::setConfig(std::vector<ServerConfig> &configs)
{
	for (std::vector<ServerConfig>::iterator configIt = configs.begin();
		 configIt != configs.end(); configIt++) {
		for (HostList::iterator hostIt = configIt->hostnames.begin();
			 hostIt != configIt->hostnames.end(); hostIt++) {
				if (*hostIt == getHost()) {
					_config = &*configIt;
					_route_config = _findMostSpecificRouteConfig(getUri());
					return true;
				}
		}
	}
	return false;
}

std::string Request::getHost() const {
    std::map<std::string, std::string>::const_iterator it = _headers.find("host");
    if (it != _headers.end()) {
        std::string host = it->second;
        std::string::size_type colonPos = host.find(':');
        if (colonPos != std::string::npos) {
            return host.substr(0, colonPos);
        }
        return host;
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
    std::memset(_error, 0, 256);
    strncpy(_error, "Request parsing error: ", 24);
	strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Request::ParsingErrorException::what() const throw() {
	return _error;
}

Request::SocketCloseException::SocketCloseException(const char *error_msg) {
	std::memset(_error, 0, 256);
    strncpy(_error, "Socket closed: ", 27);
    strncat(_error, error_msg, 256 - strlen(_error) - 1);;
}

const char *Request::SocketCloseException::what() const throw() {
    return _error;
}

std::string Request::getQueryString() const {
    size_t queryStart = _uri.find('?');
    if (queryStart != std::string::npos) {
        return _uri.substr(queryStart + 1);
    }
    return "";
}

std::string Request::RemoveQueryString(std::string uri) const {
    size_t queryEnd = uri.find('?');
    if (queryEnd != std::string::npos) {
        return uri.substr(0, queryEnd);
    }
    return uri;
}

bool Request::isTargetingCGI() const {
    const std::string& uri = getUri();
    size_t queryPos = uri.find('?');
    std::string path = (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;

    std::string lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

    bool result = false;
    if (lowerPath.length() >= 5) {
        result = lowerPath.rfind(".cgi") == lowerPath.length() - 4 ||
            lowerPath.rfind(".php") == lowerPath.length() - 4 ||
            lowerPath.rfind(".bla") == lowerPath.length() - 4;
    }
    if (result == true)
        return true;
    if (lowerPath.length() >= 4) {
        result = lowerPath.rfind(".py") == lowerPath.length() - 3;
    }

    // Always treat chunked uploads as CGI requests
    if (getHeader("transfer-encoding") == "chunked")
        return true;

    if (getMethod() != "GET")
        return true;
    return result;
}


std::string Request::getScriptPath() const {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        std::cerr << "Error getting current working directory: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to get current working directory");
    }

    std::string basePath = std::string(cwd) + _route_config->root.substr(1);
    size_t pos = _uri.rfind('/');
    if (pos == std::string::npos)
        throw ParsingErrorException(INTERRUPT, "check configuration file locations");
    std::string scriptName = _uri.substr(pos);
	std::string fullPath;
	if (_route_config->is_cgi == true) {
		fullPath = basePath + _route_config->default_file;
	} else {
		fullPath = basePath + scriptName;
		if (getMethod() == "POST" && getHeader("transfer-encoding") == "chunked")
			fullPath = std::string(cwd) + "/web/cgi/save_chunks.py";
		else if (getMethod() == "POST")
			fullPath = std::string(cwd) + "/web/cgi/upload.py";
		else if (getMethod() == "DELETE")
			fullPath = std::string(cwd) + "/web/cgi/delete.py";
		else if (getMethod() == "PUT")
			fullPath = std::string(cwd) + "/web/cgi/put.py";
	}
    return RemoveQueryString(fullPath);
}

std::string Request::getSession() const
{
    return _session_id;
}

const RouteConfig* Request::getRouteConfig() const
{
    return _route_config;
}

ServerConfig* Request::getConfig() const
{
    return _config;
}

char *Request::getBuffer() const
{
    return buffer;
}

size_t Request::getBufferLen() const
{
    return size_t(buffer_length);
}

void Request::setBufferLen(size_t len)
{
	total_read += len;
}

const char *Request::StreamingData::what() const throw()
{
    return "Part of data recieved";
}

void Request::setSessionId(std::string sessionid) {
    _session_id = sessionid;
}
