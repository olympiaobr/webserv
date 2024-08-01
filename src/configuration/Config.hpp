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
	std::string upload_path;
};

struct			ServerConfig
{
	HostList	hostnames;
	std::string root;
	std::string error_page_404;
	int			body_limit;
	std::map<std::string, RouteConfig> routes;
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

  private:
	std::string _filename;
	std::map<short, ServerConfig> _servers;

	void parseRouteConfig(RouteConfig &config, const std::string &line);
	void parseServerConfig(ServerConfig &config, const std::string &line);
};

#endif
