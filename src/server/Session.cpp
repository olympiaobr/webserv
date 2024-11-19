#include "Session.hpp"
#include <iostream>

Session::Session()
{
}

Session::Session(int socket_id): request(Request(socket_id))
{
    client_id = socket_id;
    status = S_NEW;
	std::time_t now = std::time(0);

    _client_socket = socket_id;
    std::srand(now);
	std::memset(_sessionId, 0, 100);
    std::string id = "session_id=" + utils::toString(rand() % 10000) + "_cookie";
    std::memmove(_sessionId, id.c_str(), id.size());
}

const char* Session::getSessionId() const
{
    return _sessionId;
}

Session::Session(const Session &src)
{
    request = src.request;
    response = src.response;

    std::time_t now = std::time(0);
    std::srand(now);
    std::memset(_sessionId, 0, 100);
    std::string id = "session_id=" + utils::toString(rand() % 10000) + "_cookie";
    std::memmove(_sessionId, id.c_str(), id.size());

    _client_socket = src._client_socket;
    client_id = src.client_id;
}

Session &Session::operator=(const Session &src)
{
    request = src.request;
    response = src.response;

    std::time_t now = std::time(0);
    std::srand(now);
    std::memset(_sessionId, 0, 100);
    std::string id = "session_id=" + utils::toString(rand() % 10000) + "_cookie";
    std::memmove(_sessionId, id.c_str(), id.size());

    _client_socket = src._client_socket;
    client_id = src.client_id;
    return *this;
}

void Session::recieveData()
{
    if (status != S_NEW) {
        request = Request(request, 1024 * 1024);
    }
    ssize_t bytes_read = recv(request.getSocket(), (void *)(request.buffer + request.total_read), request.getBufferLen(), 0);
    if (bytes_read == 0)
        throw Request::SocketCloseException("connection closed by client");
    else if (bytes_read < 0)
        throw Request::ParsingErrorException(Request::BAD_REQUEST, "malformed request");
    request.setBufferLen(bytes_read);
}

void Session::newRequest(std::vector<ServerConfig> &configs)
{
    request.parseHeaders();
    if (!request.setConfig(configs))
        throw Request::ParsingErrorException(Request::BAD_REQUEST, "hostname is not configured");
    response.setConfig(*request.getConfig());
    status = S_REQUEST;
    /* Check content-length */
    std::string content_length = request.getHeader("Content-Length");
    int contentLength = atoi(content_length.c_str());
    if (contentLength > request.getRouteConfig()->body_limit)
        throw Request::ParsingErrorException(Request::CONTENT_LENGTH, "content length is above limit");
    response.setStatus(200);
}

void Session::parseBody()
{
    if (request.getRouteConfig()->body_limit < request.total_read)
        throw Request::ParsingErrorException(Request::CONTENT_LENGTH, "content length is above limit");
}

int Session::getSocket() const
{
    return _client_socket;
}

void Session::sendResponse()
{
    ssize_t bytes_sent = send(getSocket(), response.getContent(), response.getContentLength(), MSG_DONTWAIT);

    response.setContent(bytes_sent);
}
