#include "session_service.h"

#include <openssl/rand.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "model/session.h"
#include "model/user.h"
#include "utils/logger.h"

static std::string nowPlusHours(int hours) {
    auto now = std::chrono::system_clock::now();
    auto later = now + std::chrono::hours(hours);
    auto laterT = std::chrono::system_clock::to_time_t(later);

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&laterT), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string SessionManager::generateSessionId() {
    unsigned char buf[32];
    if (RAND_bytes(buf, sizeof(buf)) != 1) {
        LOG_ERROR("[SESSION] RAND_bytes failed for session ID generation");
        return "";
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : buf) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

std::string SessionManager::create(int userId) {
    std::string sessionId = generateSessionId();
    if (sessionId.empty()) {
        return "";
    }

    Session session;
    session.id = sessionId;
    session.user_id = userId;
    session.expires_at = nowPlusHours(kSessionDurationHours);

    if (!SessionMapper::insert(session)) {
        LOG_ERROR("[SESSION] Failed to create session for user_id=" + std::to_string(userId));
        return "";
    }

    LOG_INFO("[SESSION] Created session for user_id=" + std::to_string(userId));
    return sessionId;
}

SessionUser SessionManager::validate(const std::string &sessionId) {
    SessionUser sessionUser;

    if (sessionId.empty()) {
        return sessionUser;
    }

    Session session = SessionMapper::findById(sessionId);
    if (session.id.empty()) {
        return sessionUser;
    }

    User user = UserMapper::findById(session.user_id);
    if (user.id == 0) {
        LOG_WARN("[SESSION] Orphaned session " + sessionId + " for deleted user " + std::to_string(session.user_id));
        SessionMapper::deleteById(sessionId);
        return sessionUser;
    }

    sessionUser.user_id = user.id;
    sessionUser.username = user.username;
    sessionUser.role = user.role;

    return sessionUser;
}

bool SessionManager::destroy(const std::string &sessionId) {
    if (sessionId.empty()) {
        return false;
    }
    bool ok = SessionMapper::deleteById(sessionId);
    if (ok) {
        LOG_INFO("[SESSION] Destroyed session " + sessionId);
    }
    return ok;
}

int SessionManager::cleanupExpired() {
    return SessionMapper::deleteExpired();
}
