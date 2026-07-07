#include <mysql/mysql.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "db/connection_pool.h"
#include "model/problem.h"
#include "model/test_case.h"
#include "model/user.h"
#include "utils/bcrypt.h"
#include "utils/config.h"

namespace {

std::string escape(MYSQL *conn, const std::string &s) {
    if (s.empty()) return "";
    std::string buf(s.size() * 2 + 1, '\0');
    mysql_real_escape_string(conn, &buf[0], s.c_str(), s.size());
    buf.resize(std::strlen(buf.c_str()));
    return buf;
}

void executeSQL(const std::string &sql) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        throw std::runtime_error("No database connection");
    }
    if (mysql_query(conn, sql.c_str()) != 0) {
        throw std::runtime_error(std::string("SQL error: ") + mysql_error(conn));
    }
}

void cleanSessions() {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) return;

    const char *sql = "DELETE FROM sessions";
    if (mysql_query(conn, sql) != 0) {
        std::cerr << "Warning: Failed to clean sessions: " << mysql_error(conn) << std::endl;
        return;
    }
    long long affected = mysql_affected_rows(conn);
    std::cout << "Cleaned sessions: " << affected << " row(s) deleted" << std::endl;
}

void cleanNonAdminUsers(const std::string &adminUsername) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) return;

    std::ostringstream sql;
    sql << "DELETE FROM users WHERE username != '"
        << escape(conn, adminUsername) << "'";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        std::cerr << "Warning: Failed to clean non-admin users: " << mysql_error(conn) << std::endl;
        return;
    }
    long long affected = mysql_affected_rows(conn);
    if (affected > 0) {
        std::cout << "Cleaned non-admin users: " << affected << " row(s) deleted" << std::endl;
    }
}

void cleanProblems() {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) return;

    const char *sql = "DELETE FROM problems";
    if (mysql_query(conn, sql) != 0) {
        std::cerr << "Warning: Failed to clean problems: " << mysql_error(conn) << std::endl;
        return;
    }
    long long affected = mysql_affected_rows(conn);
    std::cout << "Cleaned problems (and cascaded test_cases): " << affected << " row(s) deleted" << std::endl;
}

void ensureAdminUser() {
    User existing = UserMapper::findByUsername("admin");
    if (existing.id != 0) {
        std::cout << "Found existing admin user (id=" << existing.id << "), recreating..." << std::endl;
        ScopedConnection db;
        MYSQL *conn = db.get();
        if (!conn) return;

        std::ostringstream sql;
        sql << "DELETE FROM users WHERE username = '"
            << escape(conn, "admin") << "'";
        if (mysql_query(conn, sql.str().c_str()) != 0) {
            std::cerr << "Warning: Failed to delete admin: " << mysql_error(conn) << std::endl;
        }
    }

    User admin;
    admin.username = "admin";
    admin.password = bcryptHash("admin123");
    admin.role = "admin";

    int id = UserMapper::insert(admin);
    if (id < 0) {
        throw std::runtime_error("Failed to insert admin user");
    }
    std::cout << "Admin user inserted successfully (id=" << id << ")" << std::endl;
}

void insertStandardProblems() {
    Problem p1;
    p1.title = "A+B Problem";
    p1.difficulty = "Easy";
    p1.content = "# A+B Problem\n\n计算两个整数的和。\n\n## 输入\n\n一行两个整数 a, b。\n\n## 输出\n\na + b 的结果。";
    p1.code_template = "#include <iostream>\n\nint main() {\n    int a, b;\n    std::cin >> a >> b;\n    // 在此编写代码\n    return 0;\n}";
    p1.solution_code = "#include <iostream>\n\nint main() {\n    int a, b;\n    std::cin >> a >> b;\n    std::cout << a + b << std::endl;\n    return 0;\n}";
    p1.solution_content = "## 题解\n\n直接计算 a + b 即可。";
    p1.test_cases.push_back({0, 0, "1 2", "3", 0});
    p1.test_cases.push_back({0, 0, "10 20", "30", 1});
    p1.test_cases.push_back({0, 0, "-5 5", "0", 2});
    p1.test_cases.push_back({0, 0, "0 0", "0", 3});
    p1.test_cases.push_back({0, 0, "100 200", "300", 4});

    int id1 = ProblemMapper::insert(p1);
    if (id1 < 0) {
        throw std::runtime_error("Failed to insert A+B Problem");
    }
    std::cout << "Standard problem inserted: A+B Problem (id=" << id1 << ")" << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
    const char *configPath = (argc > 1) ? argv[1] : "config/config.yaml";

    Config &cfg = Config::getInstance();
    if (!cfg.load(configPath)) {
        std::cerr << "Failed to load config: " << configPath << std::endl;
        return 1;
    }

    const auto &dbCfg = cfg.database();

    try {
        ConnectionPool::getInstance().init(
            dbCfg.host, dbCfg.port,
            dbCfg.user, dbCfg.password,
            dbCfg.name, 1);
    } catch (const std::exception &e) {
        std::cerr << "Failed to initialize database pool: " << e.what() << std::endl;
        return 1;
    }

    int ret = 0;
    try {
        std::cout << "=== Database Reset ===" << std::endl;

        std::cout << "Step 1: Cleaning sessions..." << std::endl;
        cleanSessions();

        std::cout << "Step 2: Cleaning non-admin users..." << std::endl;
        cleanNonAdminUsers("admin");

        std::cout << "Step 3: Cleaning problems and test cases..." << std::endl;
        cleanProblems();

        std::cout << "Step 4: Ensuring admin user..." << std::endl;
        ensureAdminUser();

        std::cout << "Step 5: Inserting standard problems..." << std::endl;
        insertStandardProblems();

        std::cout << "=== Database reset complete ===" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        ret = 1;
    }

    ConnectionPool::getInstance().shutdown();
    return ret;
}
