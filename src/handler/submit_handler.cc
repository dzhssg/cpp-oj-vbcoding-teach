#include "submit_handler.h"

#include <nlohmann/json.hpp>

#include "service/executor_service.h"
#include "utils/logger.h"

using json = nlohmann::json;

void handleSubmit(const httplib::Request &req, httplib::Response &res) {
    LOG_DEBUG("[HANDLER] POST /api/submit");

    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::parse_error &e) {
        json err;
        err["error"] = "Invalid JSON";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("problem_id") || !body["problem_id"].is_number_integer()) {
        json err;
        err["error"] = "Missing or invalid 'problem_id'";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    if (!body.contains("code") || !body["code"].is_string()) {
        json err;
        err["error"] = "Missing or invalid 'code'";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    int problemId = body["problem_id"].get<int>();
    std::string code = body["code"].get<std::string>();

    if (code.empty()) {
        json err;
        err["error"] = "Code cannot be empty";
        res.status = 400;
        res.set_content(err.dump(), "application/json");
        return;
    }

    ExecutionResult result = ExecutorService::execute(problemId, code);

    json resp;
    resp["status"] = result.status;
    resp["passed"] = result.passed;
    resp["total"] = result.total;

    if (result.status == "CE") {
        resp["compile_error"] = result.compile_error;
    }

    json arr = json::array();
    for (const auto &r : result.results) {
        json item;
        item["position"] = r.position;
        item["status"] = r.status;
        item["input"] = r.input;
        item["expected"] = r.expected;
        item["actual"] = r.actual;
        if (!r.error.empty()) {
            item["error"] = r.error;
        }
        arr.push_back(item);
    }
    resp["results"] = arr;

    res.set_content(resp.dump(), "application/json");
}
