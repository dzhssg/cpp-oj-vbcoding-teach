#include "problem_handler.h"

#include <nlohmann/json.hpp>

#include <string>

#include "service/problem_service.h"
#include "utils/logger.h"

using json = nlohmann::json;

void handleGetProblems(const httplib::Request &req, httplib::Response &res) {
    LOG_DEBUG("[HANDLER] GET /api/problems");

    auto problems = ProblemService::getAllProblems();

    json arr = json::array();
    for (const auto &p : problems) {
        json item;
        item["id"] = p.id;
        item["title"] = p.title;
        item["difficulty"] = p.difficulty;
        arr.push_back(item);
    }

    res.set_content(arr.dump(), "application/json");
}

void handleGetProblemById(const httplib::Request &req, httplib::Response &res) {
    LOG_DEBUG("[HANDLER] GET /api/problems/:id");

    int id = std::stoi(req.matches[1].str());
    auto problem = ProblemService::getProblemById(id);

    if (problem.id == 0) {
        json err;
        err["error"] = "Problem not found";
        res.status = 404;
        res.set_content(err.dump(), "application/json");
        return;
    }

    json obj;
    obj["id"] = problem.id;
    obj["title"] = problem.title;
    obj["difficulty"] = problem.difficulty;
    obj["content"] = problem.content;
    obj["template"] = problem.code_template;
    obj["created_at"] = problem.created_at;

    json cases = json::array();
    for (const auto &tc : problem.test_cases) {
        json tc_obj;
        tc_obj["id"] = tc.id;
        tc_obj["input"] = tc.input;
        tc_obj["expected"] = tc.expected;
        tc_obj["position"] = tc.position;
        cases.push_back(tc_obj);
    }
    obj["test_cases"] = cases;

    res.set_content(obj.dump(), "application/json");
}
