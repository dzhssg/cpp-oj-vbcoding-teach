#include <gtest/gtest.h>

#include <sstream>

#include "db/connection_pool.h"
#include "model/user.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

class UserMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, 2);
    }

    void TearDown() override {
        for (const auto &name : created_usernames_) {
            deleteUser(name);
        }
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    void deleteUser(const std::string &username) {
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;
        std::ostringstream sql;
        sql << "DELETE FROM users WHERE username = '"
            << username << "'";
        mysql_query(conn, sql.str().c_str());
    }

    std::vector<std::string> created_usernames_;
};

}  // namespace

// ── Insert ────────────────────────────────────────────────────────────────

TEST_F(UserMapperTest, InsertReturnsPositiveId) {
    User u;
    u.username = "test_user_insert";
    u.password = "hashed_password";
    u.role = "user";
    created_usernames_.push_back(u.username);

    int id = UserMapper::insert(u);
    ASSERT_GT(id, 0);
}

TEST_F(UserMapperTest, InsertDefaultRoleIsUser) {
    User u;
    u.username = "test_default_role";
    u.password = "pass123";
    created_usernames_.push_back(u.username);

    int id = UserMapper::insert(u);
    ASSERT_GT(id, 0);
}

TEST_F(UserMapperTest, InsertDuplicateUsernameFails) {
    User u;
    u.username = "test_duplicate";
    u.password = "pass1";
    created_usernames_.push_back(u.username);
    int id1 = UserMapper::insert(u);
    ASSERT_GT(id1, 0);

    User u2;
    u2.username = "test_duplicate";
    u2.password = "pass2";
    int id2 = UserMapper::insert(u2);
    ASSERT_EQ(id2, -1);
}

// ── FindByUsername ────────────────────────────────────────────────────────

TEST_F(UserMapperTest, FindByUsernameReturnsCorrectData) {
    User u;
    u.username = "test_find_user";
    u.password = "secret123";
    u.role = "admin";
    created_usernames_.push_back(u.username);
    int id = UserMapper::insert(u);
    ASSERT_GT(id, 0);

    User found = UserMapper::findByUsername("test_find_user");
    ASSERT_EQ(found.id, id);
    ASSERT_EQ(found.username, "test_find_user");
    ASSERT_EQ(found.password, "secret123");
    ASSERT_EQ(found.role, "admin");
}

TEST_F(UserMapperTest, FindByUsernameNotFoundReturnsZeroId) {
    User found = UserMapper::findByUsername("nonexistent_user_xyz");
    ASSERT_EQ(found.id, 0);
}

TEST_F(UserMapperTest, FindByUsernameIsCaseInsensitive) {
    User u;
    u.username = "CaseUser";
    u.password = "pw";
    created_usernames_.push_back(u.username);
    int id = UserMapper::insert(u);
    ASSERT_GT(id, 0);

    User found = UserMapper::findByUsername("caseuser");
    ASSERT_EQ(found.id, id);
    ASSERT_EQ(found.username, "CaseUser");
}
