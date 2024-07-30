#ifndef SESSION_HPP
# define SESSION_HPP

# include <iostream>

class Session
{
	private:
		const char	*_sessionId;
		std::string	_data;
		int			_timestamp;
	public:
		Session();
		~Session();
};


#endif
