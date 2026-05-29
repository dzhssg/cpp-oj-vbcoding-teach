#ifndef PROBLEM_SERVICE_H
#define PROBLEM_SERVICE_H

#include <string>
#include <vector>

#include "model/problem.h"

class ProblemService {
public:
    ProblemService() = delete;

    static int createProblem(const Problem &problem);
    static Problem getProblemById(int id);
    static std::vector<Problem> getAllProblems();
    static bool deleteProblemById(int id);
};

#endif
