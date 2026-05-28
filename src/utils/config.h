#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
};

struct DatabaseConfig {
    std::string host = "127.0.0.1";
    int port = 3306;
    std::string user = "root";
    std::string password;
    std::string name = "cpp_oj";
    int pool_size = 4;
};

struct LoggerConfig {
    std::string level = "INFO";
    std::string file = "logs/oj.log";
};

class Config {
public:
    static Config &getInstance();

    bool load(const std::string &config_path);
    void reset();

    const ServerConfig &server() const { return server_; }
    const DatabaseConfig &database() const { return database_; }
    const LoggerConfig &logger() const { return logger_; }

private:
    Config() = default;
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;

    ServerConfig server_;
    DatabaseConfig database_;
    LoggerConfig logger_;
};

#endif
