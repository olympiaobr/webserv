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
struct ServerConfig;
typedef std::map<short, std::vector<ServerConfig> > ConfigList;

struct RouteConfig
{
	std::string root;
	std::string default_file;
	std::vector<std::string> allowed_methods;
	bool is_cgi;
	bool autoindex;
	int body_limit;
	int redirect_status_code;
	std::string redirect_url;
	std::string location;

	RouteConfig()
    : root(""), default_file(""), is_cgi(false), autoindex(false), body_limit(-1), redirect_status_code(0), redirect_url("") {}

};

struct			ServerConfig
{
	HostList hostnames;
	std::string root;
	std::map<int, std::string> error_pages;
	int			body_limit;
	std::map<std::string, RouteConfig> routes;
	int port;
	std::string formatted_body_limit;

	ServerConfig()
        : root(""), body_limit(0), port(0), formatted_body_limit("") {}
};

class Config
{
  public:
	explicit Config(const std::string &filename);
	~Config();

	Config(const Config &other);
	Config &operator=(const Config &other);

	void loadConfig();
    const ServerConfig& getServerConfig(short port, const std::string& hostname) const;
	const ConfigList &getAllServerConfigs() const;
	void addServerConfig(short port, const ServerConfig& serverConfig);
	void validateServerConfig(const ServerConfig& config) const;
	void validateRouteConfig(RouteConfig& route, const ServerConfig& server) const;

	static std::string formatSize(int bytes);

	friend std::ostream& operator<<(std::ostream& os, const RouteConfig& config);
	friend std::ostream& operator<<(std::ostream& os, const ServerConfig& config);
	friend std::ostream& operator<<(std::ostream& os, const Config& config);

	// std::map<std::string, RouteConfig> routes;

  private:
	std::string _filename;
	ConfigList _servers;

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
        : ConfigError("missing configuration setting: " + setting) {}
};

class InvalidValueError : public ConfigError {
public:
    explicit InvalidValueError(const std::string& value, const std::string& setting)
        : ConfigError("invalid value '" + value + "' for setting '" + setting + "'") {}
};

#endif
