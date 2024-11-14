#include "Session.hpp"

Session::Session()
{

}

Session::Session(int socket_id)
{
    client_id = socket_id;
	std::time_t now = std::time(0);

    _first_socket = utils::toString(socket_id);
    std::srand(now);
	std::memset(_sessionId, 0, 100);
    std::string id = "session_id=" + utils::toString(rand() % 10000) + "_cookie";
    std::memmove(_sessionId, id.c_str(), id.size());
}

const char* Session::getSessionId() const
{
    return _sessionId;
}

std::vector<Session>::const_iterator Session::findSession(std::vector<Session>& sessions, std::string &id)
{
    for (std::vector<Session>::const_iterator it = sessions.begin(); it != sessions.end(); ++it)
    {
        if (utils::strstr(it->_sessionId, id.c_str(), id.size()))
            return it;
    }
    return sessions.end();
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

    _first_socket = src._first_socket;
    client_id = src.client_id;
}
