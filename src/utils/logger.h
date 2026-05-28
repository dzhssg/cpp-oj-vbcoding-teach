#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <mutex>
#include <string>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger &getInstance();

    void init(LogLevel level, const std::string &file_path);
    void setLevel(LogLevel level);
    void shutdown();

    void debug(const std::string &msg);
    void info(const std::string &msg);
    void warning(const std::string &msg);
    void error(const std::string &msg);

    static LogLevel levelFromString(const std::string &s);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void log(LogLevel level, const std::string &msg);
    static std::string levelToString(LogLevel level);
    static std::string currentTime();

    LogLevel level_ = LogLevel::INFO;
    std::ofstream file_;
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg)  Logger::getInstance().info(msg)
#define LOG_WARN(msg)  Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

#endif
