#include "server.h"

#include "router.h"
#include "service/auth_service.h"
#include "utils/httplib.h"
#include "utils/logger.h"

void startServer(const std::string &host, int port) {
    httplib::Server server;

    server.set_mount_point("/", "./public");

    registerRoutes(server);

    AuthService::startBackgroundCleanup(1800);

    LOG_INFO("[SERVER] Listening on " + host + ":" + std::to_string(port));
    server.listen(host.c_str(), port);

    AuthService::stopBackgroundCleanup();
}
