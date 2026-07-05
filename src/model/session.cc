#include "session.h"

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

bool SessionMapper::insert(const Session &session) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[SESSION] No database connection for insert");
        return false;
    }

    std::ostringstream sql;
    sql << "INSERT INTO sessions (id, user_id, expires_at) VALUES ('"
        << escape(conn, session.id) << "', "
        << session.user_id << ", '"
        << escape(conn, session.expires_at) << "')";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[SESSION] Insert failed: " + std::string(mysql_error(conn)));
        return false;
    }

    LOG_DEBUG("[SESSION] Inserted session " + session.id);
    return true;
}

Session SessionMapper::findById(const std::string &id) {
    Session session;

    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[SESSION] No database connection for findById");
        return session;
    }

    std::ostringstream sql;
    sql << "SELECT id, user_id, expires_at, created_at "
        << "FROM sessions WHERE id = '"
        << escape(conn, id) << "' AND expires_at > NOW()";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[SESSION] findById query failed: " + std::string(mysql_error(conn)));
        return session;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return session;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        session.id = row[0] ? row[0] : "";
        session.user_id = row[1] ? std::stoi(row[1]) : 0;
        session.expires_at = row[2] ? row[2] : "";
        session.created_at = row[3] ? row[3] : "";
    }
    mysql_free_result(result);

    return session;
}

bool SessionMapper::deleteById(const std::string &id) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[SESSION] No database connection for deleteById");
        return false;
    }

    std::ostringstream sql;
    sql << "DELETE FROM sessions WHERE id = '"
        << escape(conn, id) << "'";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[SESSION] deleteById failed: " + std::string(mysql_error(conn)));
        return false;
    }

    bool deleted = mysql_affected_rows(conn) > 0;
    if (deleted) {
        LOG_DEBUG("[SESSION] Deleted session " + id);
    }
    return deleted;
}

int SessionMapper::deleteExpired() {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[SESSION] No database connection for deleteExpired");
        return -1;
    }

    const char *sql = "DELETE FROM sessions WHERE expires_at < NOW()";

    if (mysql_query(conn, sql) != 0) {
        LOG_ERROR("[SESSION] deleteExpired failed: " + std::string(mysql_error(conn)));
        return -1;
    }

    int count = static_cast<int>(mysql_affected_rows(conn));
    if (count > 0) {
        LOG_INFO("[SESSION] Cleaned up " + std::to_string(count) + " expired sessions");
    }
    return count;
}
