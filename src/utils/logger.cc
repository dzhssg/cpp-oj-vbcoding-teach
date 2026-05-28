#include "logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger &Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::init(LogLevel level, const std::string &file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;

    if (!file_path.empty()) {
        if (file_.is_open()) {
            file_.close();
        }
        file_.open(file_path, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[LOGGER] Failed to open log file: " << file_path << std::endl;
        }
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    level_ = LogLevel::INFO;
}

void Logger::debug(const std::string &msg) {
    log(LogLevel::DEBUG, msg);
}

void Logger::info(const std::string &msg) {
    log(LogLevel::INFO, msg);
}

void Logger::warning(const std::string &msg) {
    log(LogLevel::WARNING, msg);
}

void Logger::error(const std::string &msg) {
    log(LogLevel::ERROR, msg);
}

LogLevel Logger::levelFromString(const std::string &s) {
    if (s == "DEBUG") return LogLevel::DEBUG;
    if (s == "INFO")  return LogLevel::INFO;
    if (s == "WARNING") return LogLevel::WARNING;
    if (s == "ERROR") return LogLevel::ERROR;
    return LogLevel::INFO;
}

void Logger::log(LogLevel level, const std::string &msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < level_) return;

    std::ostringstream oss;
    oss << "[" << currentTime() << "] "
        << "[" << levelToString(level) << "] "
        << msg << std::endl;

    std::string line = oss.str();

    if (level >= LogLevel::WARNING) {
        std::cerr << line;
    } else {
        std::cout << line;
    }

    if (file_.is_open()) {
        file_ << line;
        file_.flush();
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
    }
    return "UNKNOWN";
}

std::string Logger::currentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::tm tm_buf;
    localtime_r(&time, &tm_buf);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}
