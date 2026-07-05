#include <gtest/gtest.h>

#include <sstream>

#include "db/connection_pool.h"
#include "model/session.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

class SessionMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, 2);
    }

    void TearDown() override {
        for (const auto &sid : created_sessions_) {
            deleteSession(sid);
        }
        for (const auto &sid : expired_sessions_) {
            deleteSession(sid);
        }
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    void deleteSession(const std::string &id) {
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;
        std::ostringstream sql;
        sql << "DELETE FROM sessions WHERE id = '" << id << "'";
        mysql_query(conn, sql.str().c_str());
    }

    void insertExpiredSession(const std::string &id, int userId, const std::string &expiresAt) {
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;
        std::ostringstream sql;
        sql << "INSERT INTO sessions (id, user_id, expires_at) VALUES ('"
            << id << "', " << userId << ", '" << expiresAt << "')";
        mysql_query(conn, sql.str().c_str());
    }

    std::vector<std::string> created_sessions_;
    std::vector<std::string> expired_sessions_;
};

}  // namespace

// ── Insert ────────────────────────────────────────────────────────────────

TEST_F(SessionMapperTest, InsertReturnsTrue) {
    Session s;
    s.id = "test_session_id_001";
    s.user_id = 1;
    s.expires_at = "2099-12-31 23:59:59";
    created_sessions_.push_back(s.id);

    bool ok = SessionMapper::insert(s);
    ASSERT_TRUE(ok);
}

TEST_F(SessionMapperTest, InsertDuplicateIdFails) {
    Session s;
    s.id = "test_dup_session";
    s.user_id = 1;
    s.expires_at = "2099-12-31 23:59:59";
    created_sessions_.push_back(s.id);

    ASSERT_TRUE(SessionMapper::insert(s));

    Session s2;
    s2.id = "test_dup_session";
    s2.user_id = 2;
    s2.expires_at = "2099-12-31 23:59:59";
    ASSERT_FALSE(SessionMapper::insert(s2));
}

// ── FindById ──────────────────────────────────────────────────────────────

TEST_F(SessionMapperTest, FindByIdReturnsCorrectData) {
    Session s;
    s.id = "test_find_session";
    s.user_id = 42;
    s.expires_at = "2099-12-31 23:59:59";
    created_sessions_.push_back(s.id);
    ASSERT_TRUE(SessionMapper::insert(s));

    Session found = SessionMapper::findById("test_find_session");
    ASSERT_EQ(found.id, "test_find_session");
    ASSERT_EQ(found.user_id, 42);
    ASSERT_EQ(found.expires_at, "2099-12-31 23:59:59");
    ASSERT_FALSE(found.created_at.empty());
}

TEST_F(SessionMapperTest, FindByIdNotFoundReturnsEmpty) {
    Session found = SessionMapper::findById("nonexistent_session_xyz");
    ASSERT_TRUE(found.id.empty());
    ASSERT_EQ(found.user_id, 0);
}

TEST_F(SessionMapperTest, FindByIdExcludesExpired) {
    expired_sessions_.push_back("test_expired_session");
    insertExpiredSession("test_expired_session", 1, "2020-01-01 00:00:00");

    Session found = SessionMapper::findById("test_expired_session");
    ASSERT_TRUE(found.id.empty());
}

// ── DeleteById ────────────────────────────────────────────────────────────

TEST_F(SessionMapperTest, DeleteByIdRemovesExisting) {
    Session s;
    s.id = "test_delete_session";
    s.user_id = 1;
    s.expires_at = "2099-12-31 23:59:59";
    created_sessions_.push_back(s.id);
    ASSERT_TRUE(SessionMapper::insert(s));

    ASSERT_TRUE(SessionMapper::deleteById("test_delete_session"));

    Session found = SessionMapper::findById("test_delete_session");
    ASSERT_TRUE(found.id.empty());
}

TEST_F(SessionMapperTest, DeleteByIdNonExistentReturnsFalse) {
    ASSERT_FALSE(SessionMapper::deleteById("no_such_session"));
}

// ── DeleteExpired ─────────────────────────────────────────────────────────

TEST_F(SessionMapperTest, DeleteExpiredRemovesExpiredSessions) {
    expired_sessions_.push_back("expired_1");
    expired_sessions_.push_back("expired_2");
    insertExpiredSession("expired_1", 1, "2020-01-01 00:00:00");
    insertExpiredSession("expired_2", 2, "2020-01-01 00:00:00");

    int removed = SessionMapper::deleteExpired();
    ASSERT_GE(removed, 2);
}

TEST_F(SessionMapperTest, DeleteExpiredPreservesActive) {
    Session s;
    s.id = "active_session_for_cleanup";
    s.user_id = 1;
    s.expires_at = "2099-12-31 23:59:59";
    created_sessions_.push_back(s.id);
    ASSERT_TRUE(SessionMapper::insert(s));

    SessionMapper::deleteExpired();

    Session found = SessionMapper::findById("active_session_for_cleanup");
    ASSERT_EQ(found.id, "active_session_for_cleanup");
}

TEST_F(SessionMapperTest, DeleteExpiredReturnsZeroWhenNone) {
    int removed = SessionMapper::deleteExpired();
    ASSERT_GE(removed, 0);
}
