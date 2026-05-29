#include "test_case.h"

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

int TestCaseMapper::insert(const TestCase &tc) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[TESTCASE] No database connection for insert");
        return -1;
    }

    std::ostringstream sql;
    sql << "INSERT INTO test_cases (problem_id, input, expected, position) VALUES ("
        << tc.problem_id << ", '"
        << escape(conn, tc.input) << "', '"
        << escape(conn, tc.expected) << "', "
        << tc.position << ")";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[TESTCASE] Insert failed: " + std::string(mysql_error(conn)));
        return -1;
    }

    return static_cast<int>(mysql_insert_id(conn));
}

std::vector<TestCase> TestCaseMapper::findByProblemId(int problem_id) {
    std::vector<TestCase> cases;

    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[TESTCASE] No database connection for findByProblemId");
        return cases;
    }

    std::ostringstream sql;
    sql << "SELECT id, problem_id, input, expected, position "
        << "FROM test_cases WHERE problem_id = " << problem_id
        << " ORDER BY position, id";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[TESTCASE] findByProblemId query failed: "
                  + std::string(mysql_error(conn)));
        return cases;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return cases;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        TestCase tc;
        tc.id = std::stoi(row[0]);
        tc.problem_id = std::stoi(row[1]);
        tc.input = row[2] ? row[2] : "";
        tc.expected = row[3] ? row[3] : "";
        tc.position = std::stoi(row[4]);
        cases.push_back(tc);
    }
    mysql_free_result(result);

    return cases;
}

bool TestCaseMapper::deleteByProblemId(int problem_id) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[TESTCASE] No database connection for deleteByProblemId");
        return false;
    }

    std::ostringstream sql;
    sql << "DELETE FROM test_cases WHERE problem_id = " << problem_id;

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[TESTCASE] deleteByProblemId failed: "
                  + std::string(mysql_error(conn)));
        return false;
    }

    return static_cast<int>(mysql_affected_rows(conn)) > 0;
}
