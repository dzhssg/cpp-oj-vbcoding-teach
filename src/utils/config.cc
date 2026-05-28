#include "config.h"

#include <yaml-cpp/yaml.h>

#include <iostream>

Config &Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::load(const std::string &config_path) {
    try {
        YAML::Node root = YAML::LoadFile(config_path);

        if (root["server"]) {
            auto s = root["server"];
            if (s["host"]) server_.host = s["host"].as<std::string>();
            if (s["port"]) server_.port = s["port"].as<int>();
        }

        if (root["database"]) {
            auto d = root["database"];
            if (d["host"])      database_.host = d["host"].as<std::string>();
            if (d["port"])      database_.port = d["port"].as<int>();
            if (d["user"])      database_.user = d["user"].as<std::string>();
            if (d["password"])  database_.password = d["password"].as<std::string>();
            if (d["name"])      database_.name = d["name"].as<std::string>();
            if (d["pool_size"]) database_.pool_size = d["pool_size"].as<int>();
        }

        if (root["logger"]) {
            auto l = root["logger"];
            if (l["level"]) logger_.level = l["level"].as<std::string>();
            if (l["file"])  logger_.file = l["file"].as<std::string>();
        }

        return true;
    } catch (const YAML::Exception &e) {
        std::cerr << "[CONFIG] Failed to load config file: " << config_path
                  << " (" << e.what() << ")" << std::endl;
        return false;
    }
}

void Config::reset() {
    server_ = ServerConfig{};
    database_ = DatabaseConfig{};
    logger_ = LoggerConfig{};
}
