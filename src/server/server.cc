#include "server.h"

#include "router.h"
#include "utils/httplib.h"
#include "utils/logger.h"

void startServer(const std::string &host, int port) {
    httplib::Server server;

    // Mount static files from public/ directory
    server.set_mount_point("/", "./public");

    registerRoutes(server);

    LOG_INFO("[SERVER] Listening on " + host + ":" + std::to_string(port));
    server.listen(host.c_str(), port);
}
