#include "admin_handler.h"

#include <nlohmann/json.hpp>

#include <string>

#include "handler/auth_handler.h"
#include "model/problem.h"
#include "model/test_case.h"
#include "service/problem_service.h"
#include "utils/logger.h"

using json = nlohmann::json;

void handleAdminCreateProblem(const httplib::Request &req, httplib::Response &res) {
    LOG_DEBUG("[ADMIN] POST /api/admin/problems");

    SessionUser user;
    if (!requireAdmin(req, res, user)) {
        return;
    }

    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::parse_error &e) {
        LOG_WARN("[ADMIN] Invalid JSON in request body");
        json err;
        err["error"] = "Invalid JSON";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("title") || !body["title"].is_string()) {
        json err;
        err["error"] = "Missing or invalid 'title' field";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("content") || !body["content"].is_string()) {
        json err;
        err["error"] = "Missing or invalid 'content' field";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    Problem problem;
    problem.title = body["title"].get<std::string>();
    problem.difficulty = body.value("difficulty", "Easy");
    problem.content = body["content"].get<std::string>();
    problem.code_template = body.value("template", "");

    if (body.contains("test_cases") && body["test_cases"].is_array()) {
        for (const auto &tc_json : body["test_cases"]) {
            TestCase tc;
            tc.input = tc_json.value("input", "");
            tc.expected = tc_json.value("expected", "");
            tc.position = tc_json.value("position", static_cast<int>(problem.test_cases.size()));
            problem.test_cases.push_back(tc);
        }
    }

    int id = ProblemService::createProblem(problem);
    if (id < 0) {
        json err;
        err["error"] = "Failed to create problem";
        res.status = 500;
        res.set_content(err.dump(), "application/json");
        return;
    }

    json resp;
    resp["id"] = id;
    resp["message"] = "Problem created successfully";
    res.status = 201;
    res.set_content(resp.dump(), "application/json");
}

void handleAdminDeleteProblem(const httplib::Request &req, httplib::Response &res) {
    LOG_DEBUG("[ADMIN] DELETE /api/admin/problems/:id");

    SessionUser user;
    if (!requireAdmin(req, res, user)) {
        return;
    }

    int id = std::stoi(req.matches[1].str());

    if (!ProblemService::deleteProblemById(id)) {
        json err;
        err["error"] = "Problem not found";
        res.status = 404;
        res.set_content(err.dump(), "application/json");
        return;
    }

    json resp;
    resp["message"] = "Problem deleted successfully";
    res.set_content(resp.dump(), "application/json");
}
