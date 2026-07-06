#include "problem.h"

#include <mysql/mysql.h>

#include <cstring>
#include <sstream>

#include "db/connection_pool.h"
#include "model/test_case.h"
#include "utils/logger.h"

namespace {

std::string escape(MYSQL *conn, const std::string &s) {
    if (s.empty()) return "";
    std::string buf(s.size() * 2 + 1, '\0');
    mysql_real_escape_string(conn, &buf[0], s.c_str(), s.size());
    buf.resize(std::strlen(buf.c_str()));
    return buf;
}

std::string strOrNull(const std::string &s) {
    return s.empty() ? "NULL" : "'" + s + "'";
}

}  // namespace

int ProblemMapper::insert(const Problem &problem) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[PROBLEM] No database connection for insert");
        return -1;
    }

    std::ostringstream sql;
    sql << "INSERT INTO problems (title, difficulty, content, `template`, solution_content, solution_code) VALUES ('"
        << escape(conn, problem.title) << "', '"
        << escape(conn, problem.difficulty) << "', '"
        << escape(conn, problem.content) << "', '"
        << escape(conn, problem.code_template) << "', '"
        << escape(conn, problem.solution_content) << "', '"
        << escape(conn, problem.solution_code) << "')";

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[PROBLEM] Insert failed: " + std::string(mysql_error(conn)));
        return -1;
    }

    int id = static_cast<int>(mysql_insert_id(conn));

    for (const auto &tc : problem.test_cases) {
        TestCase c = tc;
        c.problem_id = id;
        if (TestCaseMapper::insert(c) < 0) {
            LOG_WARN("[PROBLEM] Failed to insert test case for problem " + std::to_string(id));
        }
    }

    LOG_INFO("[PROBLEM] Inserted problem id=" + std::to_string(id)
             + " with " + std::to_string(problem.test_cases.size()) + " test cases");
    return id;
}

bool ProblemMapper::update(const Problem &problem) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[PROBLEM] No database connection for update");
        return false;
    }

    std::ostringstream sql;
    sql << "UPDATE problems SET "
        << "title = '" << escape(conn, problem.title) << "', "
        << "difficulty = '" << escape(conn, problem.difficulty) << "', "
        << "content = '" << escape(conn, problem.content) << "', "
        << "`template` = '" << escape(conn, problem.code_template) << "', "
        << "solution_content = '" << escape(conn, problem.solution_content) << "', "
        << "solution_code = '" << escape(conn, problem.solution_code) << "' "
        << "WHERE id = " << problem.id;

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[PROBLEM] Update failed: " + std::string(mysql_error(conn)));
        return false;
    }

    if (static_cast<int>(mysql_affected_rows(conn)) == 0) {
        LOG_WARN("[PROBLEM] No problem found with id=" + std::to_string(problem.id));
        return false;
    }

    if (!problem.test_cases.empty()) {
        std::ostringstream delSql;
        delSql << "DELETE FROM test_cases WHERE problem_id = " << problem.id;
        mysql_query(conn, delSql.str().c_str());

        for (const auto &tc : problem.test_cases) {
            TestCase c = tc;
            c.problem_id = problem.id;
            if (TestCaseMapper::insert(c) < 0) {
                LOG_WARN("[PROBLEM] Failed to insert test case during update");
            }
        }
        LOG_INFO("[PROBLEM] Updated " + std::to_string(problem.test_cases.size())
                 + " test cases for problem id=" + std::to_string(problem.id));
    }

    LOG_INFO("[PROBLEM] Updated problem id=" + std::to_string(problem.id));
    return true;
}

Problem ProblemMapper::findById(int id) {
    Problem problem;

    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[PROBLEM] No database connection for findById");
        return problem;
    }

    std::ostringstream sql;
    sql << "SELECT id, title, difficulty, content, `template`, solution_content, solution_code, created_at "
        << "FROM problems WHERE id = " << id;

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[PROBLEM] findById query failed: " + std::string(mysql_error(conn)));
        return problem;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return problem;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        problem.id = std::stoi(row[0]);
        problem.title = row[1] ? row[1] : "";
        problem.difficulty = row[2] ? row[2] : "";
        problem.content = row[3] ? row[3] : "";
        problem.code_template = row[4] ? row[4] : "";
        problem.solution_content = row[5] ? row[5] : "";
        problem.solution_code = row[6] ? row[6] : "";
        problem.created_at = row[7] ? row[7] : "";
    }
    mysql_free_result(result);

    if (problem.id > 0) {
        problem.test_cases = TestCaseMapper::findByProblemId(problem.id);
    }

    return problem;
}

std::vector<Problem> ProblemMapper::findAll() {
    std::vector<Problem> problems;

    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[PROBLEM] No database connection for findAll");
        return problems;
    }

    const char *sql = "SELECT id, title, difficulty FROM problems ORDER BY id";
    if (mysql_query(conn, sql) != 0) {
        LOG_ERROR("[PROBLEM] findAll query failed: " + std::string(mysql_error(conn)));
        return problems;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return problems;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Problem p;
        p.id = std::stoi(row[0]);
        p.title = row[1] ? row[1] : "";
        p.difficulty = row[2] ? row[2] : "";
        problems.push_back(std::move(p));
    }
    mysql_free_result(result);

    return problems;
}

bool ProblemMapper::deleteById(int id) {
    ScopedConnection db;
    MYSQL *conn = db.get();
    if (!conn) {
        LOG_ERROR("[PROBLEM] No database connection for deleteById");
        return false;
    }

    std::ostringstream sql;
    sql << "DELETE FROM problems WHERE id = " << id;

    if (mysql_query(conn, sql.str().c_str()) != 0) {
        LOG_ERROR("[PROBLEM] deleteById failed: " + std::string(mysql_error(conn)));
        return false;
    }

    LOG_INFO("[PROBLEM] Deleted problem id=" + std::to_string(id));
    return static_cast<int>(mysql_affected_rows(conn)) > 0;
}
