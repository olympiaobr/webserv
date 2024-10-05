#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <map>
# include <string>
# include <vector>
# include <iomanip>
# include <cmath>
#include <stdexcept>
#include <sstream>

typedef std::vector<std::string> HostList;

struct			RouteConfig
{
	std::string root;
	std::string default_file;
	std::vector<std::string> allowed_methods;
	bool is_cgi;
	bool autoindex;
	int redirect_status_code;
	std::string redirect_url;

	RouteConfig() : is_cgi(false), autoindex(false) {}
};

struct			ServerConfig
{
	HostList	hostnames;
	std::string root;
	std::map<int, std::string> error_pages;
	int			body_limit;
	std::map<std::string, RouteConfig> routes;
	int port;
	std::string formatted_body_limit;
};

class Config
{
  public:
	explicit Config(const std::string &filename);
	~Config();

	Config(const Config &other);
	Config &operator=(const Config &other);

	void loadConfig();
	const ServerConfig &getServerConfig(short port) const;
	const std::map<short, ServerConfig> &getAllServerConfigs() const;
	void addServerConfig(short port, const ServerConfig &serverConfig);
	void validateServerConfig(const ServerConfig& config) const;
	void validateRouteConfig(const RouteConfig& route) const;

	static std::string formatSize(int bytes);

	friend std::ostream& operator<<(std::ostream& os, const RouteConfig& config);
	friend std::ostream& operator<<(std::ostream& os, const ServerConfig& config);
	friend std::ostream& operator<<(std::ostream& os, const Config& config);

  private:
	std::string _filename;
	std::map<short, ServerConfig> _servers;

	void _parseRouteConfig(RouteConfig &config, const std::string &line);
	void _parseServerConfig(ServerConfig &config, const std::string &line);
};

class ConfigError : public std::logic_error {
public:
    explicit ConfigError(const std::string& message)
        : std::logic_error(message) {}
};

class MissingSettingError : public ConfigError {
public:
    explicit MissingSettingError(const std::string& setting)
        : ConfigError("Missing configuration setting: " + setting) {}
};

class InvalidValueError : public ConfigError {
public:
    explicit InvalidValueError(const std::string& value, const std::string& setting)
        : ConfigError("Invalid value '" + value + "' for setting '" + setting + "'") {}
};

#endif
