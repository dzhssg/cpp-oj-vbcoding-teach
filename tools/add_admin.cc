#include <mysql/mysql.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "db/connection_pool.h"
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

void deleteUserByUsername(const std::string &username) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        throw std::runtime_error("No database connection for delete");
    }

    std::ostringstream sql;
    sql << "DELETE FROM users WHERE username = '"
        << escape(conn, username) << "'";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        std::string err =
            "Delete failed: " + std::string(mysql_error(conn));
        throw std::runtime_error(err);
    }

    long long affected = mysql_affected_rows(conn);
    if (affected > 0) {
        std::cout << "Existing admin user deleted (" << affected
                  << " row(s) affected)" << std::endl;
    }
}

}  // namespace

int main(int argc, char *argv[]) {
    const char *configPath =
        (argc > 1) ? argv[1] : "config/config.yaml";

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
        std::cerr << "Failed to initialize database pool: " << e.what()
                  << std::endl;
        return 1;
    }

    int ret = 0;
    try {
        User existing = UserMapper::findByUsername("admin");
        if (existing.id != 0) {
            std::cout << "Found existing admin user (id=" << existing.id
                      << "), deleting..." << std::endl;
            deleteUserByUsername("admin");
        }

        User admin;
        admin.username = "admin";
        admin.password = bcryptHash("admin123");
        admin.role = "admin";

        int id = UserMapper::insert(admin);
        if (id < 0) {
            std::cerr << "Failed to insert admin user" << std::endl;
            ret = 1;
        } else {
            std::cout << "Admin user inserted successfully (id=" << id
                      << ")" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        ret = 1;
    }

    ConnectionPool::getInstance().shutdown();
    return ret;
}
