#include "user.h"

#include <mysql/mysql.h>

#include <cstring>
#include <sstream>

#include "db/connection_pool.h"
#include "utils/logger.h"

namespace {

std::string escape(MYSQL *conn, const std::string &s) {
    if (s.empty()) return "";
    std::string buf(s.size() * 2 + 1, '\0');
    mysql_real_escape_string(conn, &buf[0], s.c_str(), s.size());
    buf.resize(std::strlen(buf.c_str()));
    return buf;
}

}  // namespace

int UserMapper::insert(const User &user) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[USER] No database connection for insert");
        return -1;
    }

    std::ostringstream sql;
    sql << "INSERT INTO users (username, password, role) VALUES ('"
        << escape(conn, user.username) << "', '"
        << escape(conn, user.password) << "', '"
        << escape(conn, user.role.empty() ? "user" : user.role) << "')";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[USER] Insert failed: " + std::string(mysql_error(conn)));
        return -1;
    }

    int id = static_cast<int>(mysql_insert_id(conn));
    LOG_INFO("[USER] Inserted user id=" + std::to_string(id));
    return id;
}

User UserMapper::findById(int id) {
    User user;

    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[USER] No database connection for findById");
        return user;
    }

    std::ostringstream sql;
    sql << "SELECT id, username, password, role, created_at "
        << "FROM users WHERE id = " << id;

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[USER] findById query failed: "
                  + std::string(mysql_error(conn)));
        return user;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return user;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        user.id = std::stoi(row[0]);
        user.username = row[1] ? row[1] : "";
        user.password = row[2] ? row[2] : "";
        user.role = row[3] ? row[3] : "";
        user.created_at = row[4] ? row[4] : "";
    }
    mysql_free_result(result);

    return user;
}

User UserMapper::findByUsername(const std::string &username) {
    User user;

    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[USER] No database connection for findByUsername");
        return user;
    }

    std::ostringstream sql;
    sql << "SELECT id, username, password, role, created_at "
        << "FROM users WHERE username = '"
        << escape(conn, username) << "'";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[USER] findByUsername query failed: "
                  + std::string(mysql_error(conn)));
        return user;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return user;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        user.id = std::stoi(row[0]);
        user.username = row[1] ? row[1] : "";
        user.password = row[2] ? row[2] : "";
        user.role = row[3] ? row[3] : "";
        user.created_at = row[4] ? row[4] : "";
    }
    mysql_free_result(result);

    return user;
}
