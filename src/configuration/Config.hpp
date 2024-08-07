#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <map>
# include <string>
# include <vector>


typedef std::vector<std::string> HostList;

struct			RouteConfig
{
	std::string root;
	std::string default_file;
	std::vector<std::string> allowed_methods;
	bool is_cgi;
	bool autoindex;

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
};

class Config
{
  public:
	explicit Config(const std::string &filename);
	~Config();

	Config(const Config &other);
	Config &operator=(const Config &other);
	bool loadConfig();
	const ServerConfig &getServerConfig(short port) const;
	const std::map<short, ServerConfig> &getAllServerConfigs() const;

	void addServerConfig(short port, const ServerConfig &serverConfig);

	friend std::ostream& operator<<(std::ostream& os, const RouteConfig& config);
	friend std::ostream& operator<<(std::ostream& os, const ServerConfig& config);
	friend std::ostream& operator<<(std::ostream& os, const Config& config);

  private:
	std::string _filename;
	std::map<short, ServerConfig> _servers;

	void parseRouteConfig(RouteConfig &config, const std::string &line);
	void parseServerConfig(ServerConfig &config, const std::string &line);
};

#endif
