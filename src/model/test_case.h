#ifndef TEST_CASE_H
#define TEST_CASE_H

#include <string>
#include <vector>

struct TestCase {
    int id = 0;
    int problem_id = 0;
    std::string input;
    std::string expected;
    int position = 0;
};

class TestCaseMapper {
public:
    TestCaseMapper() = delete;

    static int insert(const TestCase &tc);
    static std::vector<TestCase> findByProblemId(int problem_id);
    static bool deleteByProblemId(int problem_id);
};

#endif
