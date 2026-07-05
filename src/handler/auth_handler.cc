#include "auth_handler.h"

#include <nlohmann/json.hpp>

#include <sstream>

#include "model/user.h"
#include "service/auth_service.h"
#include "utils/bcrypt.h"
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

void handleRegister(const httplib::Request &req, httplib::Response &res) {
    LOG_DEBUG("[AUTH] POST /api/register");

    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::parse_error &e) {
        json err;
        err["error"] = "Invalid JSON";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("username") || !body["username"].is_string()) {
        json err;
        err["error"] = "Missing or invalid 'username' field";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("password") || !body["password"].is_string()) {
        json err;
        err["error"] = "Missing or invalid 'password' field";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    std::string username = body["username"].get<std::string>();
    std::string password = body["password"].get<std::string>();

    if (username.empty()) {
        json err;
        err["error"] = "Username cannot be empty";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (username.size() < 3 || username.size() > 64) {
        json err;
        err["error"] = "Username must be between 3 and 64 characters";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (password.empty()) {
        json err;
        err["error"] = "Password cannot be empty";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (password.size() < 6) {
        json err;
        err["error"] = "Password must be at least 6 characters";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (body.contains("confirm_password") && body["confirm_password"].is_string()) {
        std::string confirm = body["confirm_password"].get<std::string>();
        if (confirm != password) {
            json err;
            err["error"] = "Passwords do not match";
            res.status = 400;
            res.set_content(err.dump(), "application/json");
            return;
        }
    }

    User existing = UserMapper::findByUsername(username);
    if (existing.id != 0) {
        json err;
        err["error"] = "Username already taken";
        res.status = 409;
        res.set_content(err.dump(), "application/json");
        return;
    }

    User newUser;
    newUser.username = username;
    newUser.password = bcryptHash(password);
    newUser.role = "user";

    int userId = UserMapper::insert(newUser);
    if (userId < 0) {
        json err;
        err["error"] = "Failed to create user";
        res.status = 500;
        res.set_content(err.dump(), "application/json");
        return;
    }

    json resp;
    resp["id"] = userId;
    resp["message"] = "User registered successfully";
    res.status = 201;
    res.set_content(resp.dump(), "application/json");
}

void handleLogin(const httplib::Request &req, httplib::Response &res) {
    LOG_DEBUG("[AUTH] POST /api/login");

    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::parse_error &e) {
        json err;
        err["error"] = "Invalid JSON";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("username") || !body["username"].is_string()) {
        json err;
        err["error"] = "Missing or invalid 'username' field";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("password") || !body["password"].is_string()) {
        json err;
        err["error"] = "Missing or invalid 'password' field";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    std::string username = body["username"].get<std::string>();
    std::string password = body["password"].get<std::string>();

    if (username.empty()) {
        json err;
        err["error"] = "Username cannot be empty";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (password.empty()) {
        json err;
        err["error"] = "Password cannot be empty";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    User user = UserMapper::findByUsername(username);
    if (user.id == 0) {
        json err;
        err["error"] = "Invalid username or password";
        res.status = 401;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!bcryptVerify(password, user.password)) {
        json err;
        err["error"] = "Invalid username or password";
        res.status = 401;
        res.set_content(err.dump(), "application/json");
        return;
    }

    std::string sessionId = AuthService::createSession(user.id);
    if (sessionId.empty()) {
        json err;
        err["error"] = "Failed to create session";
        res.status = 500;
        res.set_content(err.dump(), "application/json");
        return;
    }

    setSessionCookie(res, sessionId);

    json resp;
    resp["id"] = user.id;
    resp["username"] = user.username;
    resp["role"] = user.role;
    res.status = 200;
    res.set_content(resp.dump(), "application/json");
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
