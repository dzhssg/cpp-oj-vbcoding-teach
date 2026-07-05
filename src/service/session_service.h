#ifndef SESSION_SERVICE_H
#define SESSION_SERVICE_H

#include <string>

struct SessionUser {
    int user_id = 0;
    std::string username;
    std::string role;
};

class SessionManager {
public:
    SessionManager() = delete;

    static std::string create(int userId);

    static SessionUser validate(const std::string &sessionId);

    static bool destroy(const std::string &sessionId);

    static int cleanupExpired();

private:
    static std::string generateSessionId();
    static constexpr int kSessionDurationHours = 24;
};

#endif
