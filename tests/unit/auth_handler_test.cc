#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>

#include "db/connection_pool.h"
#include "handler/auth_handler.h"
#include "model/user.h"
#include "service/auth_service.h"
#include "service/session_service.h"
#include "utils/bcrypt.h"
#include "utils/logger.h"

using json = nlohmann::json;

namespace {

// ── extractSessionId (no DB) ──────────────────────────────────────────────

TEST(ExtractSessionIdTest, ExtractsFromCookieHeader) {
    httplib::Request req;
    req.set_header("Cookie", "session_id=abc123");

    ASSERT_EQ(extractSessionId(req), "abc123");
}

TEST(ExtractSessionIdTest, ReturnsEmptyWhenNoCookie) {
    httplib::Request req;
    ASSERT_EQ(extractSessionId(req), "");
}

TEST(ExtractSessionIdTest, ReturnsEmptyWhenCookieEmpty) {
    httplib::Request req;
    req.set_header("Cookie", "");

    ASSERT_EQ(extractSessionId(req), "");
}

TEST(ExtractSessionIdTest, ReturnsEmptyWhenKeyNotFound) {
    httplib::Request req;
    req.set_header("Cookie", "other_key=value");

    ASSERT_EQ(extractSessionId(req), "");
}

TEST(ExtractSessionIdTest, ExtractsFromMultipleCookies) {
    httplib::Request req;
    req.set_header("Cookie", "cookie_a=val_a; session_id=my_session; cookie_b=val_b");

    ASSERT_EQ(extractSessionId(req), "my_session");
}

TEST(ExtractSessionIdTest, IgnoresMalformedCookies) {
    httplib::Request req;
    req.set_header("Cookie", "no_equals; session_id=valid_id");

    ASSERT_EQ(extractSessionId(req), "valid_id");
}

// ── setSessionCookie ──────────────────────────────────────────────────────

TEST(SetSessionCookieTest, SetsCorrectHeader) {
    httplib::Response res;
    setSessionCookie(res, "test_session_abc");

    std::string header = res.get_header_value("Set-Cookie");
    ASSERT_NE(header.find("session_id=test_session_abc"), std::string::npos);
    ASSERT_NE(header.find("Path=/"), std::string::npos);
    ASSERT_NE(header.find("HttpOnly"), std::string::npos);
    ASSERT_NE(header.find("SameSite=Lax"), std::string::npos);
    ASSERT_NE(header.find("Max-Age=" + std::to_string(24 * 60 * 60)), std::string::npos);
}

// ── clearSessionCookie ────────────────────────────────────────────────────

TEST(ClearSessionCookieTest, SetsExpiredCookie) {
    httplib::Response res;
    clearSessionCookie(res);

    std::string header = res.get_header_value("Set-Cookie");
    ASSERT_NE(header.find("session_id=;"), std::string::npos);
    ASSERT_NE(header.find("Max-Age=0"), std::string::npos);
    ASSERT_NE(header.find("Path=/"), std::string::npos);
    ASSERT_NE(header.find("HttpOnly"), std::string::npos);
    ASSERT_NE(header.find("SameSite=Lax"), std::string::npos);
}

// ── authenticate / requireAdmin (needs DB) ────────────────────────────────

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

class AuthHandlerDBTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, 2);
        userId_ = createTestUser("auth_test_user", "user");
        adminId_ = createTestUser("auth_test_admin", "admin");
    }

    void TearDown() override {
        for (const auto &sid : session_ids_) {
            deleteSession(sid);
        }
        deleteUser("auth_test_user");
        deleteUser("auth_test_admin");
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    int createTestUser(const std::string &username, const std::string &role) {
        User u;
        u.username = username;
        u.password = "hashed_pass";
        u.role = role;
        return UserMapper::insert(u);
    }

    void deleteUser(const std::string &username) {
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;
        std::ostringstream sql;
        sql << "DELETE FROM users WHERE username = '" << username << "'";
        mysql_query(conn, sql.str().c_str());
    }

    void deleteSession(const std::string &id) {
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;
        std::ostringstream sql;
        sql << "DELETE FROM sessions WHERE id = '" << id << "'";
        mysql_query(conn, sql.str().c_str());
    }

    int userId_ = 0;
    int adminId_ = 0;
    std::vector<std::string> session_ids_;
};

TEST_F(AuthHandlerDBTest, AuthenticateSucceedsWithValidSession) {
    ASSERT_GT(userId_, 0);

    std::string sid = SessionManager::create(userId_);
    session_ids_.push_back(sid);
    ASSERT_FALSE(sid.empty());

    httplib::Request req;
    req.set_header("Cookie", "session_id=" + sid);
    httplib::Response res;
    SessionUser user;

    ASSERT_TRUE(authenticate(req, res, user));
    ASSERT_EQ(user.user_id, userId_);
    ASSERT_EQ(user.username, "auth_test_user");
    ASSERT_EQ(user.role, "user");
}

TEST_F(AuthHandlerDBTest, AuthenticateFailsWithNoCookie) {
    httplib::Request req;
    httplib::Response res;
    SessionUser user;

    ASSERT_FALSE(authenticate(req, res, user));
    ASSERT_EQ(res.status, 401);
}

TEST_F(AuthHandlerDBTest, AuthenticateFailsWithInvalidSession) {
    httplib::Request req;
    req.set_header("Cookie", "session_id=invalid_session_abc");
    httplib::Response res;
    SessionUser user;

    ASSERT_FALSE(authenticate(req, res, user));
    ASSERT_EQ(res.status, 401);
}

TEST_F(AuthHandlerDBTest, RequireAdminSucceedsForAdmin) {
    ASSERT_GT(adminId_, 0);

    std::string sid = SessionManager::create(adminId_);
    session_ids_.push_back(sid);
    ASSERT_FALSE(sid.empty());

    httplib::Request req;
    req.set_header("Cookie", "session_id=" + sid);
    httplib::Response res;
    SessionUser user;

    ASSERT_TRUE(requireAdmin(req, res, user));
    ASSERT_EQ(user.role, "admin");
}

TEST_F(AuthHandlerDBTest, RequireAdminFailsForNonAdmin) {
    ASSERT_GT(userId_, 0);

    std::string sid = SessionManager::create(userId_);
    session_ids_.push_back(sid);
    ASSERT_FALSE(sid.empty());

    httplib::Request req;
    req.set_header("Cookie", "session_id=" + sid);
    httplib::Response res;
    SessionUser user;

    ASSERT_FALSE(requireAdmin(req, res, user));
    ASSERT_EQ(res.status, 403);
}

// ── handleLogin (needs DB) ────────────────────────────────────────────────

class LoginHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, 2);
    }

    void TearDown() override {
        for (const auto &sid : session_ids_) {
            deleteSession(sid);
        }
        for (const auto &name : created_usernames_) {
            deleteUser(name);
        }
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    int createUserWithPassword(const std::string &username,
                                const std::string &password,
                                const std::string &role = "user") {
        User u;
        u.username = username;
        u.password = bcryptHash(password);
        u.role = role;
        created_usernames_.push_back(username);
        return UserMapper::insert(u);
    }

    void deleteUser(const std::string &username) {
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;
        std::ostringstream sql;
        sql << "DELETE FROM users WHERE username = '" << username << "'";
        mysql_query(conn, sql.str().c_str());
    }

    void deleteSession(const std::string &id) {
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;
        std::ostringstream sql;
        sql << "DELETE FROM sessions WHERE id = '" << id << "'";
        mysql_query(conn, sql.str().c_str());
    }

    static std::string extractSessionIdFromSetCookie(const std::string &header) {
        if (header.empty()) return "";
        auto eq = header.find('=');
        if (eq == std::string::npos) return "";
        auto semi = header.find(';', eq);
        if (semi == std::string::npos) return header.substr(eq + 1);
        return header.substr(eq + 1, semi - eq - 1);
    }

    void trackSessionFromResponse(const httplib::Response &res) {
        std::string cookie = res.get_header_value("Set-Cookie");
        if (!cookie.empty()) {
            std::string sid = extractSessionIdFromSetCookie(cookie);
            if (!sid.empty()) {
                session_ids_.push_back(sid);
            }
        }
    }

    static httplib::Request makeLoginReq(const std::string &username,
                                          const std::string &password) {
        httplib::Request req;
        json body;
        body["username"] = username;
        body["password"] = password;
        req.body = body.dump();
        req.set_header("Content-Type", "application/json");
        return req;
    }

    static httplib::Request makeLoginReqRaw(const std::string &rawBody) {
        httplib::Request req;
        req.body = rawBody;
        req.set_header("Content-Type", "application/json");
        return req;
    }

    static json parseResponseBody(const httplib::Response &res) {
        return json::parse(res.body);
    }

    std::vector<std::string> session_ids_;
    std::vector<std::string> created_usernames_;
};

// ── Basic login flow ──────────────────────────────────────────────────────

TEST_F(LoginHandlerTest, LoginSucceedsWithCorrectCredentials) {
    int userId = createUserWithPassword("login_user", "testPass123");
    ASSERT_GT(userId, 0);

    httplib::Request req = makeLoginReq("login_user", "testPass123");
    httplib::Response res;

    handleLogin(req, res);
    trackSessionFromResponse(res);

    ASSERT_EQ(res.status, 200);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["id"].get<int>(), userId);
    ASSERT_EQ(body["username"].get<std::string>(), "login_user");
    ASSERT_EQ(body["role"].get<std::string>(), "user");
}

TEST_F(LoginHandlerTest, LoginFailsWithWrongPassword) {
    int userId = createUserWithPassword("wrong_pw_user", "correctPass");
    ASSERT_GT(userId, 0);

    httplib::Request req = makeLoginReq("wrong_pw_user", "wrongPass");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 401);

    json body = parseResponseBody(res);
    ASSERT_TRUE(body.contains("error"));
    ASSERT_EQ(body["error"].get<std::string>(), "Invalid username or password");
}

TEST_F(LoginHandlerTest, LoginFailsWithNonExistentUser) {
    httplib::Request req = makeLoginReq("nonexistent_user", "somePass");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 401);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["error"].get<std::string>(), "Invalid username or password");
}

TEST_F(LoginHandlerTest, LoginSucceedsForAdminUser) {
    int adminId = createUserWithPassword("login_admin", "adminPass", "admin");
    ASSERT_GT(adminId, 0);

    httplib::Request req = makeLoginReq("login_admin", "adminPass");
    httplib::Response res;

    handleLogin(req, res);
    trackSessionFromResponse(res);

    ASSERT_EQ(res.status, 200);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["id"].get<int>(), adminId);
    ASSERT_EQ(body["role"].get<std::string>(), "admin");
}

// ── Input validation ──────────────────────────────────────────────────────

TEST_F(LoginHandlerTest, LoginFailsWithEmptyUsername) {
    httplib::Request req = makeLoginReq("", "somePass");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["error"].get<std::string>(), "Username cannot be empty");
}

TEST_F(LoginHandlerTest, LoginFailsWithEmptyPassword) {
    httplib::Request req = makeLoginReq("someUser", "");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["error"].get<std::string>(), "Password cannot be empty");
}

TEST_F(LoginHandlerTest, LoginFailsWithMissingUsernameField) {
    httplib::Request req = makeLoginReqRaw(R"({"password":"pass123"})");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["error"].get<std::string>(), "Missing or invalid 'username' field");
}

TEST_F(LoginHandlerTest, LoginFailsWithMissingPasswordField) {
    httplib::Request req = makeLoginReqRaw(R"({"username":"user1"})");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["error"].get<std::string>(), "Missing or invalid 'password' field");
}

TEST_F(LoginHandlerTest, LoginFailsWithInvalidJson) {
    httplib::Request req = makeLoginReqRaw("not valid json");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["error"].get<std::string>(), "Invalid JSON");
}

TEST_F(LoginHandlerTest, LoginFailsWithEmptyBody) {
    httplib::Request req = makeLoginReqRaw("");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);
}

TEST_F(LoginHandlerTest, LoginFailsWithNonStringUsername) {
    httplib::Request req = makeLoginReqRaw(R"({"username":123,"password":"pass"})");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["error"].get<std::string>(), "Missing or invalid 'username' field");
}

TEST_F(LoginHandlerTest, LoginFailsWithNonStringPassword) {
    httplib::Request req = makeLoginReqRaw(R"({"username":"user1","password":true})");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);
}

// ── Password verification (within login flow) ─────────────────────────────

TEST_F(LoginHandlerTest, PasswordVerificationIsCaseSensitive) {
    createUserWithPassword("case_user", "MyPassword");
    httplib::Response res;

    handleLogin(makeLoginReq("case_user", "MyPassword"), res);
    trackSessionFromResponse(res);
    ASSERT_EQ(res.status, 200);

    handleLogin(makeLoginReq("case_user", "mypassword"), res);
    ASSERT_EQ(res.status, 401);
}

TEST_F(LoginHandlerTest, PasswordVerificationHandlesSpecialCharacters) {
    createUserWithPassword("special_user", "p@$$!<>&*()");
    httplib::Response res;

    handleLogin(makeLoginReq("special_user", "p@$$!<>&*()"), res);
    trackSessionFromResponse(res);
    ASSERT_EQ(res.status, 200);
}

TEST_F(LoginHandlerTest, PasswordVerificationHandlesUnicode) {
    std::string pass = u8"密码测试日本語";
    createUserWithPassword("unicode_user", pass);
    httplib::Response res;

    handleLogin(makeLoginReq("unicode_user", pass), res);
    trackSessionFromResponse(res);
    ASSERT_EQ(res.status, 200);

    handleLogin(makeLoginReq("unicode_user", u8"错误密码"), res);
    ASSERT_EQ(res.status, 401);
}

TEST_F(LoginHandlerTest, PasswordVerificationHandlesEmptyPassword) {
    createUserWithPassword("empty_pw_user", "");
    httplib::Response res;

    handleLogin(makeLoginReq("empty_pw_user", ""), res);
    ASSERT_EQ(res.status, 400);
}

TEST_F(LoginHandlerTest, PasswordVerificationHandlesOnlyWhitespace) {
    createUserWithPassword("ws_user", "   ");
    httplib::Response res;

    handleLogin(makeLoginReq("ws_user", "   "), res);
    trackSessionFromResponse(res);
    ASSERT_EQ(res.status, 200);

    handleLogin(makeLoginReq("ws_user", "wrong"), res);
    ASSERT_EQ(res.status, 401);
}

TEST_F(LoginHandlerTest, PasswordVerificationHandlesLongPassword) {
    std::string longPass(72, 'x');
    createUserWithPassword("longpw_user", longPass);
    httplib::Response res;

    handleLogin(makeLoginReq("longpw_user", longPass), res);
    trackSessionFromResponse(res);
    ASSERT_EQ(res.status, 200);

    std::string wrongLong(72, 'y');
    handleLogin(makeLoginReq("longpw_user", wrongLong), res);
    ASSERT_EQ(res.status, 401);
}

TEST_F(LoginHandlerTest, PasswordVerificationHandlesNullByte) {
    std::string pass("abc\0xyz", 7);
    createUserWithPassword("nullbyte_user", pass);
    httplib::Response res;

    handleLogin(makeLoginReq("nullbyte_user", std::string("abc\0xyz", 7)), res);
    trackSessionFromResponse(res);
    ASSERT_EQ(res.status, 200);

    handleLogin(makeLoginReq("nullbyte_user", std::string("abc\0ooo", 7)), res);
    ASSERT_EQ(res.status, 401);
}

// ── Session cookie management ─────────────────────────────────────────────

TEST_F(LoginHandlerTest, LoginSetsSessionCookieOnSuccess) {
    createUserWithPassword("cookie_user", "pass123");
    httplib::Request req = makeLoginReq("cookie_user", "pass123");
    httplib::Response res;

    handleLogin(req, res);
    trackSessionFromResponse(res);

    ASSERT_EQ(res.status, 200);
    ASSERT_TRUE(res.has_header("Set-Cookie"));

    std::string cookie = res.get_header_value("Set-Cookie");
    ASSERT_NE(cookie.find("session_id="), std::string::npos);
    ASSERT_NE(cookie.find("Path=/"), std::string::npos);
    ASSERT_NE(cookie.find("HttpOnly"), std::string::npos);
    ASSERT_NE(cookie.find("SameSite=Lax"), std::string::npos);
    ASSERT_NE(cookie.find("Max-Age=" + std::to_string(24 * 60 * 60)), std::string::npos);
}

TEST_F(LoginHandlerTest, LoginDoesNotSetCookieOnFailure) {
    httplib::Request req = makeLoginReq("nobody", "nopass");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 401);
    ASSERT_FALSE(res.has_header("Set-Cookie"));
}

TEST_F(LoginHandlerTest, LoginCreatesValidSessionThatCanBeValidated) {
    int userId = createUserWithPassword("validate_user", "pass456");
    ASSERT_GT(userId, 0);

    httplib::Response res;
    handleLogin(makeLoginReq("validate_user", "pass456"), res);
    trackSessionFromResponse(res);

    ASSERT_EQ(res.status, 200);

    std::string sid = extractSessionIdFromSetCookie(res.get_header_value("Set-Cookie"));
    ASSERT_FALSE(sid.empty());

    SessionUser sessionUser = SessionManager::validate(sid);
    ASSERT_EQ(sessionUser.user_id, userId);
    ASSERT_EQ(sessionUser.username, "validate_user");
    ASSERT_EQ(sessionUser.role, "user");
}

TEST_F(LoginHandlerTest, LoginDoesNotCreateSessionOnFailedAuth) {
    createUserWithPassword("no_session_user", "correct");
    httplib::Response res;

    handleLogin(makeLoginReq("no_session_user", "wrong"), res);

    ASSERT_EQ(res.status, 401);
    ASSERT_FALSE(res.has_header("Set-Cookie"));
}

TEST_F(LoginHandlerTest, LoginCreatesUniqueSessionsOnMultipleLogins) {
    createUserWithPassword("multi_login", "pass789");
    httplib::Response res1, res2;

    handleLogin(makeLoginReq("multi_login", "pass789"), res1);
    trackSessionFromResponse(res1);
    ASSERT_EQ(res1.status, 200);

    handleLogin(makeLoginReq("multi_login", "pass789"), res2);
    trackSessionFromResponse(res2);
    ASSERT_EQ(res2.status, 200);

    std::string sid1 = extractSessionIdFromSetCookie(res1.get_header_value("Set-Cookie"));
    std::string sid2 = extractSessionIdFromSetCookie(res2.get_header_value("Set-Cookie"));
    ASSERT_FALSE(sid1.empty());
    ASSERT_FALSE(sid2.empty());
    ASSERT_NE(sid1, sid2) << "Each login should create a unique session ID";
}

// ── Edge cases ────────────────────────────────────────────────────────────

TEST_F(LoginHandlerTest, LoginHandlesExtraJsonFields) {
    createUserWithPassword("extra_user", "pass");
    httplib::Request req = makeLoginReqRaw(
        R"({"username":"extra_user","password":"pass","extra":"ignored"})");
    httplib::Response res;

    handleLogin(req, res);
    trackSessionFromResponse(res);

    ASSERT_EQ(res.status, 200);
}

TEST_F(LoginHandlerTest, LoginHandlesUsernameWithWhitespace) {
    createUserWithPassword("user with space", "pass");
    httplib::Response res;

    handleLogin(makeLoginReq("user with space", "pass"), res);
    trackSessionFromResponse(res);
    ASSERT_EQ(res.status, 200);
}

TEST_F(LoginHandlerTest, LoginFailsForNonexistentUserEvenWithEmptyPassword) {
    httplib::Request req = makeLoginReq("nobody_here", "");
    httplib::Response res;

    handleLogin(req, res);

    ASSERT_EQ(res.status, 400);
}

// ── handleLogout ───────────────────────────────────────────────────────────

TEST_F(LoginHandlerTest, LogoutClearsSessionCookie) {
    createUserWithPassword("logout_user", "pass123");

    httplib::Response loginRes;
    handleLogin(makeLoginReq("logout_user", "pass123"), loginRes);
    trackSessionFromResponse(loginRes);
    ASSERT_EQ(loginRes.status, 200);

    std::string sid = extractSessionIdFromSetCookie(
        loginRes.get_header_value("Set-Cookie"));
    ASSERT_FALSE(sid.empty());

    httplib::Request logoutReq;
    logoutReq.set_header("Cookie", "session_id=" + sid);
    httplib::Response logoutRes;
    handleLogout(logoutReq, logoutRes);

    ASSERT_EQ(logoutRes.status, 200);

    json body = parseResponseBody(logoutRes);
    ASSERT_EQ(body["message"].get<std::string>(), "Logged out successfully");

    std::string cookie = logoutRes.get_header_value("Set-Cookie");
    ASSERT_NE(cookie.find("session_id=;"), std::string::npos);
    ASSERT_NE(cookie.find("Max-Age=0"), std::string::npos);
}

TEST_F(LoginHandlerTest, LogoutInvalidatesSession) {
    createUserWithPassword("invalidate_user", "pass456");

    httplib::Response loginRes;
    handleLogin(makeLoginReq("invalidate_user", "pass456"), loginRes);
    ASSERT_EQ(loginRes.status, 200);

    std::string sid = extractSessionIdFromSetCookie(
        loginRes.get_header_value("Set-Cookie"));
    ASSERT_FALSE(sid.empty());

    SessionUser validUser = SessionManager::validate(sid);
    ASSERT_GT(validUser.user_id, 0);

    httplib::Request logoutReq;
    logoutReq.set_header("Cookie", "session_id=" + sid);
    httplib::Response logoutRes;
    handleLogout(logoutReq, logoutRes);
    ASSERT_EQ(logoutRes.status, 200);

    SessionUser invalidUser = SessionManager::validate(sid);
    ASSERT_EQ(invalidUser.user_id, 0);
}

TEST_F(LoginHandlerTest, LogoutSucceedsWithoutSessionCookie) {
    httplib::Request req;
    httplib::Response res;
    handleLogout(req, res);

    ASSERT_EQ(res.status, 200);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["message"].get<std::string>(), "Logged out successfully");

    std::string cookie = res.get_header_value("Set-Cookie");
    ASSERT_NE(cookie.find("session_id=;"), std::string::npos);
    ASSERT_NE(cookie.find("Max-Age=0"), std::string::npos);
}

TEST_F(LoginHandlerTest, LogoutSucceedsWithInvalidSessionId) {
    httplib::Request req;
    req.set_header("Cookie", "session_id=nonexistent_session_xyz");
    httplib::Response res;

    handleLogout(req, res);

    ASSERT_EQ(res.status, 200);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["message"].get<std::string>(), "Logged out successfully");
}

TEST_F(LoginHandlerTest, LogoutSucceedsWithEmptySessionCookie) {
    httplib::Request req;
    req.set_header("Cookie", "session_id=");
    httplib::Response res;

    handleLogout(req, res);

    ASSERT_EQ(res.status, 200);

    json body = parseResponseBody(res);
    ASSERT_EQ(body["message"].get<std::string>(), "Logged out successfully");
}

}  // namespace
