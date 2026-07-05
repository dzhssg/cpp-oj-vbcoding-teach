#include "auth_handler.h"

#include <nlohmann/json.hpp>
#include <sstream>

#include "utils/logger.h"

using json = nlohmann::json;

std::string extractSessionId(const httplib::Request &req) {
    std::string raw = req.get_header_value("Cookie");
    if (raw.empty()) return "";

    std::istringstream stream(raw);
    std::string token;
    while (std::getline(stream, token, ';')) {
        auto pos = token.find('=');
        if (pos == std::string::npos) continue;

        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);

        while (!key.empty() && key.front() == ' ') key.erase(0, 1);
        while (!key.empty() && key.back() == ' ') key.pop_back();
        while (!value.empty() && value.front() == ' ') value.erase(0, 1);
        while (!value.empty() && value.back() == ' ') value.pop_back();

        if (key == kSessionCookieName) {
            return value;
        }
    }
    return "";
}

static constexpr int kMaxAgeSec = 24 * 60 * 60;

void setSessionCookie(httplib::Response &res, const std::string &sessionId) {
    std::ostringstream oss;
    oss << kSessionCookieName << "=" << sessionId
        << "; Path=/"
        << "; HttpOnly"
        << "; SameSite=Lax"
        << "; Max-Age=" << kMaxAgeSec;
    res.set_header("Set-Cookie", oss.str());
}

void clearSessionCookie(httplib::Response &res) {
    std::ostringstream oss;
    oss << kSessionCookieName << "=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0";
    res.set_header("Set-Cookie", oss.str());
}

bool authenticate(const httplib::Request &req, httplib::Response &res, SessionUser &user) {
    std::string sessionId = extractSessionId(req);
    if (sessionId.empty()) {
        json err;
        err["error"] = "Authentication required";
        res.status = 401;
        res.set_content(err.dump(), "application/json");
        return false;
    }

    user = SessionManager::validate(sessionId);
    if (user.user_id == 0) {
        json err;
        err["error"] = "Invalid or expired session";
        res.status = 401;
        res.set_content(err.dump(), "application/json");
        return false;
    }

    return true;
}

bool requireAdmin(const httplib::Request &req, httplib::Response &res, SessionUser &user) {
    if (!authenticate(req, res, user)) {
        return false;
    }

    if (user.role != "admin") {
        json err;
        err["error"] = "Admin privileges required";
        res.status = 403;
        res.set_content(err.dump(), "application/json");
        return false;
    }

    return true;
}
