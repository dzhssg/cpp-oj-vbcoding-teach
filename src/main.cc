#include <csignal>
#include <cstdlib>
#include <iostream>

#include "db/connection_pool.h"
#include "server/server.h"
#include "utils/config.h"
#include "utils/logger.h"

volatile sig_atomic_t running = 1;

void signalHandler(int) {
    running = 0;
}

int main() {
    // Load configuration
    if (!Config::getInstance().load("config/config.yaml")) {
        std::cerr << "Failed to load config/config.yaml" << std::endl;
        return EXIT_FAILURE;
    }

    const auto &serverCfg = Config::getInstance().server();
    const auto &dbCfg = Config::getInstance().database();
    const auto &loggerCfg = Config::getInstance().logger();

    // Initialize logger
    Logger::getInstance().init(
        Logger::levelFromString(loggerCfg.level),
        loggerCfg.file);

    LOG_INFO("[MAIN] Starting cpp-oj server...");

    // Initialize database connection pool
    ConnectionPool::getInstance().init(
        dbCfg.host, dbCfg.port,
        dbCfg.user, dbCfg.password,
        dbCfg.name, dbCfg.pool_size);

    LOG_INFO("[MAIN] Database connection pool initialized");

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Start HTTP server
    startServer(serverCfg.host, serverCfg.port);

    // Shutdown
    LOG_INFO("[MAIN] Shutting down...");
    ConnectionPool::getInstance().shutdown();
    Logger::getInstance().shutdown();

    return EXIT_SUCCESS;
}
