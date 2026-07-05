#include "auth_service.h"

#include <mysql/mysql.h>
#include <openssl/rand.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "db/connection_pool.h"
#include "model/session.h"
#include "utils/logger.h"

std::string AuthService::generateSessionId() {
    unsigned char buf[32];
    if (RAND_bytes(buf, sizeof(buf)) != 1) {
        LOG_ERROR("[AUTH] RAND_bytes failed for session id generation");
        return "";
    }

    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<int>(buf[i]);
    }
    return oss.str();
}

std::string AuthService::createSession(int userId) {
    std::string sessionId = generateSessionId();
    if (sessionId.empty()) {
        LOG_ERROR("[AUTH] Failed to generate session id");
        return "";
    }

    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::hours(24);
    std::time_t expiresTime = std::chrono::system_clock::to_time_t(expires);

    std::ostringstream expiresStr;
    expiresStr << std::put_time(std::gmtime(&expiresTime), "%Y-%m-%d %H:%M:%S");

    Session session;
    session.id = sessionId;
    session.user_id = userId;
    session.expires_at = expiresStr.str();

    if (!SessionMapper::insert(session)) {
        LOG_ERROR("[AUTH] Failed to insert session for user_id=" + std::to_string(userId));
        return "";
    }

    LOG_INFO("[AUTH] Session created for user_id=" + std::to_string(userId));
    return sessionId;
}

User AuthService::validateSession(const std::string &sessionId) {
    User user;

    if (sessionId.empty()) {
        LOG_WARN("[AUTH] Empty session id");
        return user;
    }

    Session session = SessionMapper::findById(sessionId);
    if (session.id.empty()) {
        LOG_WARN("[AUTH] Session not found: " + sessionId.substr(0, 8) + "...");
        return user;
    }

    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[AUTH] No database connection for session expiry check");
        return user;
    }

    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM sessions WHERE id = '"
        << sessionId << "' AND expires_at > NOW()";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[AUTH] Session expiry check query failed: "
                  + std::string(mysql_error(conn)));
        return user;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return user;

    MYSQL_ROW row = mysql_fetch_row(result);
    int count = (row && row[0]) ? std::stoi(row[0]) : 0;
    mysql_free_result(result);

    if (count == 0) {
        LOG_WARN("[AUTH] Session expired: " + sessionId.substr(0, 8) + "...");
        SessionMapper::deleteById(sessionId);
        return user;
    }

    user = UserMapper::findById(session.user_id);
    if (user.id == 0) {
        LOG_WARN("[AUTH] User not found for session user_id="
                 + std::to_string(session.user_id));
    }

    return user;
}

bool AuthService::destroySession(const std::string &sessionId) {
    if (sessionId.empty()) {
        return false;
    }

    return SessionMapper::deleteById(sessionId);
}

void AuthService::cleanupExpiredSessions() {
    SessionMapper::deleteExpired();
}
