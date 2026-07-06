#include "problem_service.h"

#include "model/problem.h"
#include "utils/logger.h"

int ProblemService::createProblem(const Problem &problem) {
    if (problem.title.empty()) {
        LOG_WARN("[SERVICE] Problem title is empty");
        return -1;
    }
    if (problem.content.empty()) {
        LOG_WARN("[SERVICE] Problem content is empty");
        return -1;
    }
    return ProblemMapper::insert(problem);
}

bool ProblemService::updateProblem(const Problem &problem) {
    if (problem.id <= 0) {
        LOG_WARN("[SERVICE] Invalid problem id for update");
        return false;
    }
    if (problem.title.empty()) {
        LOG_WARN("[SERVICE] Problem title is empty");
        return false;
    }
    if (problem.content.empty()) {
        LOG_WARN("[SERVICE] Problem content is empty");
        return false;
    }
    return ProblemMapper::update(problem);
}

Problem ProblemService::getProblemById(int id) {
    auto problem = ProblemMapper::findById(id);
    if (problem.id == 0) {
        LOG_WARN("[SERVICE] Problem not found: id=" + std::to_string(id));
    }
    return problem;
}

std::vector<Problem> ProblemService::getAllProblems() {
    return ProblemMapper::findAll();
}

bool ProblemService::deleteProblemById(int id) {
    if (!ProblemMapper::deleteById(id)) {
        LOG_WARN("[SERVICE] Failed to delete problem: id=" + std::to_string(id));
        return false;
    }
    return true;
}
