#include "auth_service.h"

#include <chrono>

#include "utils/logger.h"

std::thread AuthService::cleanupThread_;
std::atomic<bool> AuthService::cleanupRunning_{false};

std::string AuthService::createSession(int userId) {
    return SessionManager::create(userId);
}

SessionUser AuthService::validateSession(const std::string &sessionId) {
    return SessionManager::validate(sessionId);
}

void AuthService::destroySession(const std::string &sessionId) {
    SessionManager::destroy(sessionId);
}

int AuthService::cleanupExpiredSessions() {
    return SessionManager::cleanupExpired();
}

void AuthService::startBackgroundCleanup(int intervalSeconds) {
    if (cleanupRunning_) {
        LOG_WARN("[AUTH] Background session cleanup already running");
        return;
    }

    cleanupRunning_ = true;
    cleanupThread_ = std::thread(cleanupLoop, intervalSeconds);

    LOG_INFO("[AUTH] Started background session cleanup (interval="
             + std::to_string(intervalSeconds) + "s)");
}

void AuthService::stopBackgroundCleanup() {
    if (!cleanupRunning_) return;

    cleanupRunning_ = false;
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }

    LOG_INFO("[AUTH] Stopped background session cleanup");
}

void AuthService::cleanupLoop(int intervalSeconds) {
    using namespace std::chrono;

    while (cleanupRunning_) {
        auto now = steady_clock::now();
        auto waitUntil = now + seconds(intervalSeconds);

        while (cleanupRunning_ && steady_clock::now() < waitUntil) {
            std::this_thread::sleep_for(seconds(1));
        }

        if (!cleanupRunning_) break;

        int count = SessionManager::cleanupExpired();
        if (count > 0) {
            LOG_INFO("[AUTH] Cleaned up " + std::to_string(count) + " expired sessions");
        }
    }
}
