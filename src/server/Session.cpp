#include "Session.hpp"

Session::Session(int socket_id)
{
	std::time_t now = std::time(0);

    _first_socket = utils::toString(socket_id);
    std::srand(now);
	std::memset(_sessionId, 0, 100);
    std::string id = utils::toString(rand() % 10000) + "_cookie";
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
