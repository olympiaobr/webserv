# include "Utils.hpp"

std::string &utils::toLowerCase(std::string &string)
{
	for (size_t i = 0; i < string.size(); ++i) {
		string[i] = tolower(string[i]);
	}
	return string;
}
