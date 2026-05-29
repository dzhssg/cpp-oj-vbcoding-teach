#ifndef PROBLEM_H
#define PROBLEM_H

#include <string>
#include <vector>

#include "test_case.h"

struct Problem {
    int id = 0;
    std::string title;
    std::string difficulty;
    std::string content;
    std::string code_template;
    std::string created_at;

    std::vector<TestCase> test_cases;
};

class ProblemMapper {
public:
    ProblemMapper() = delete;

    static int insert(const Problem &problem);
    static Problem findById(int id);
    static std::vector<Problem> findAll();
    static bool deleteById(int id);
};

#endif
