#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <atomic>
#include <string>
#include <thread>

#include "session_service.h"

class AuthService {
public:
    AuthService() = delete;

    static std::string createSession(int userId);
    static SessionUser validateSession(const std::string &sessionId);
    static void destroySession(const std::string &sessionId);
    static int cleanupExpiredSessions();

    static void startBackgroundCleanup(int intervalSeconds = 1800);
    static void stopBackgroundCleanup();

private:
    static void cleanupLoop(int intervalSeconds);

    static std::thread cleanupThread_;
    static std::atomic<bool> cleanupRunning_;
};

#endif
