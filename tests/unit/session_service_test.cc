#include <gtest/gtest.h>

#include <sstream>

#include "db/connection_pool.h"
#include "model/user.h"
#include "service/session_service.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

class SessionServiceTest : public ::testing::Test {
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

    int createTestUser(const std::string &username, const std::string &role = "user") {
        User u;
        u.username = username;
        u.password = "hashed_pass";
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

    std::vector<std::string> session_ids_;
    std::vector<std::string> created_usernames_;
};

}  // namespace

// ── Create ────────────────────────────────────────────────────────────────

TEST_F(SessionServiceTest, CreateReturnsNonEmptyId) {
    int userId = createTestUser("session_create_user");
    ASSERT_GT(userId, 0);

    std::string sid = SessionManager::create(userId);
    session_ids_.push_back(sid);

    ASSERT_FALSE(sid.empty());
    ASSERT_EQ(sid.size(), 64u);
}

TEST_F(SessionServiceTest, CreateGeneratesUniqueIds) {
    int userId = createTestUser("session_unique_user");
    ASSERT_GT(userId, 0);

    std::string sid1 = SessionManager::create(userId);
    std::string sid2 = SessionManager::create(userId);
    session_ids_.push_back(sid1);
    session_ids_.push_back(sid2);

    ASSERT_FALSE(sid1.empty());
    ASSERT_FALSE(sid2.empty());
    ASSERT_NE(sid1, sid2);
}

TEST_F(SessionServiceTest, CreateWithNonExistentUserFails) {
    std::string sid = SessionManager::create(999999);
    ASSERT_TRUE(sid.empty());
}

// ── Validate ──────────────────────────────────────────────────────────────

TEST_F(SessionServiceTest, ValidateReturnsCorrectUser) {
    int userId = createTestUser("session_validate_user", "admin");
    ASSERT_GT(userId, 0);

    std::string sid = SessionManager::create(userId);
    session_ids_.push_back(sid);
    ASSERT_FALSE(sid.empty());

    SessionUser user = SessionManager::validate(sid);
    ASSERT_EQ(user.user_id, userId);
    ASSERT_EQ(user.username, "session_validate_user");
    ASSERT_EQ(user.role, "admin");
}

TEST_F(SessionServiceTest, ValidateEmptyIdReturnsEmpty) {
    SessionUser user = SessionManager::validate("");
    ASSERT_EQ(user.user_id, 0);
    ASSERT_TRUE(user.username.empty());
    ASSERT_TRUE(user.role.empty());
}

TEST_F(SessionServiceTest, ValidateNonExistentIdReturnsEmpty) {
    SessionUser user = SessionManager::validate("nonexistent_session_abcdef");
    ASSERT_EQ(user.user_id, 0);
}

TEST_F(SessionServiceTest, ValidateOrphanedSessionReturnsEmpty) {
    int userId = createTestUser("session_orphan_user");
    ASSERT_GT(userId, 0);

    std::string sid = SessionManager::create(userId);
    session_ids_.push_back(sid);
    ASSERT_FALSE(sid.empty());

    deleteUser("session_orphan_user");
    created_usernames_.clear();

    SessionUser user = SessionManager::validate(sid);
    ASSERT_EQ(user.user_id, 0);
}

// ── Destroy ───────────────────────────────────────────────────────────────

TEST_F(SessionServiceTest, DestroyRemovesSession) {
    int userId = createTestUser("session_destroy_user");
    ASSERT_GT(userId, 0);

    std::string sid = SessionManager::create(userId);
    session_ids_.push_back(sid);
    ASSERT_FALSE(sid.empty());

    ASSERT_TRUE(SessionManager::destroy(sid));

    SessionUser user = SessionManager::validate(sid);
    ASSERT_EQ(user.user_id, 0);
}

TEST_F(SessionServiceTest, DestroyEmptyIdReturnsFalse) {
    ASSERT_FALSE(SessionManager::destroy(""));
}

TEST_F(SessionServiceTest, DestroyNonExistentIdReturnsFalse) {
    ASSERT_FALSE(SessionManager::destroy("nonexistent_session_xyz"));
}

// ── CleanupExpired ────────────────────────────────────────────────────────

TEST_F(SessionServiceTest, CleanupExpiredRemovesExpiredSessions) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    ASSERT_NE(conn, nullptr);

    int userId = createTestUser("session_cleanup_user");
    ASSERT_GT(userId, 0);

    std::ostringstream sql;
    sql << "INSERT INTO sessions (id, user_id, expires_at) VALUES "
        << "('expired_session_1', " << userId << ", '2020-01-01 00:00:00'), "
        << "('expired_session_2', " << userId << ", '2020-01-01 00:00:00')";
    mysql_query(conn, sql.str().c_str());
    session_ids_.push_back("expired_session_1");
    session_ids_.push_back("expired_session_2");

    int removed = SessionManager::cleanupExpired();
    ASSERT_GE(removed, 2);
}

// ── Integration: create -> validate -> destroy ────────────────────────────

TEST_F(SessionServiceTest, FullSessionLifecycle) {
    int userId = createTestUser("session_lifecycle_user");
    ASSERT_GT(userId, 0);

    std::string sid = SessionManager::create(userId);
    session_ids_.push_back(sid);
    ASSERT_FALSE(sid.empty());

    SessionUser user = SessionManager::validate(sid);
    ASSERT_EQ(user.user_id, userId);

    ASSERT_TRUE(SessionManager::destroy(sid));

    SessionUser gone = SessionManager::validate(sid);
    ASSERT_EQ(gone.user_id, 0);
}
