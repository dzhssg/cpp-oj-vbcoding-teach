#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "db/connection_pool.h"
#include "model/problem.h"
#include "model/test_case.h"
#include "service/executor_service.h"
#include "service/problem_service.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

const char *kCorrectCode = R"(#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}
)";

const char *kWrongCode = R"(#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a - b << std::endl;
    return 0;
}
)";

const char *kCompileErrorCode = R"(#include <iostream>
int main() {
    std::cout << undefined_variable << std::endl;
    return 0;
}
)";

const char *kRuntimeErrorCode = R"(#include <iostream>
int main() {
    int *p = nullptr;
    *p = 42;
    return 0;
}
)";

const char *kStringCode = R"(#include <iostream>
#include <string>
int main() {
    std::string line;
    std::getline(std::cin, line);
    std::cout << line << std::endl;
    return 0;
}
)";

class ExecutorServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, 2);

        Problem p;
        p.title = "A+B Problem (Test)";
        p.difficulty = "Easy";
        p.content = "Compute a+b";

        TestCase tc1;
        tc1.input = "1 2\n";
        tc1.expected = "3\n";
        tc1.position = 0;

        TestCase tc2;
        tc2.input = "10 20\n";
        tc2.expected = "30\n";
        tc2.position = 1;

        TestCase tc3;
        tc3.input = "-5 8\n";
        tc3.expected = "3\n";
        tc3.position = 2;

        p.test_cases = {tc1, tc2, tc3};

        problem_id_ = ProblemMapper::insert(p);
        ASSERT_GT(problem_id_, 0);
    }

    void TearDown() override {
        if (problem_id_ > 0) {
            ProblemMapper::deleteById(problem_id_);
        }
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    int problem_id_ = 0;
};

}  // namespace

TEST_F(ExecutorServiceTest, CorrectCodeAllAccepted) {
    ExecutionResult result = ExecutorService::execute(problem_id_, kCorrectCode);

    ASSERT_EQ(result.status, "AC");
    ASSERT_EQ(result.total, 3);
    ASSERT_EQ(result.passed, 3);
    ASSERT_EQ(result.results.size(), 3u);

    for (const auto &r : result.results) {
        ASSERT_EQ(r.status, "AC");
    }
}

TEST_F(ExecutorServiceTest, WrongAnswerDetected) {
    ExecutionResult result = ExecutorService::execute(problem_id_, kWrongCode);

    ASSERT_EQ(result.status, "WA");
    ASSERT_EQ(result.total, 3);
    ASSERT_LT(result.passed, result.total);

    bool has_wa = false;
    for (const auto &r : result.results) {
        if (r.status == "WA") has_wa = true;
    }
    ASSERT_TRUE(has_wa);
}

TEST_F(ExecutorServiceTest, CompileErrorDetected) {
    ExecutionResult result = ExecutorService::execute(problem_id_, kCompileErrorCode);

    ASSERT_EQ(result.status, "CE");
    ASSERT_FALSE(result.compile_error.empty());
    ASSERT_EQ(result.total, 0);
    ASSERT_EQ(result.passed, 0);
}

TEST_F(ExecutorServiceTest, RuntimeErrorDetected) {
    ExecutionResult result = ExecutorService::execute(problem_id_, kRuntimeErrorCode);

    ASSERT_NE(result.status, "AC");
    ASSERT_EQ(result.total, 3);
}

TEST_F(ExecutorServiceTest, NonExistentProblem) {
    ExecutionResult result = ExecutorService::execute(999999, kCorrectCode);

    ASSERT_EQ(result.status, "CE");
    ASSERT_FALSE(result.compile_error.empty());
}

TEST_F(ExecutorServiceTest, StringInputOutput) {
    Problem p;
    p.title = "Echo Test";
    p.difficulty = "Easy";
    p.content = "Echo input";

    TestCase tc;
    tc.input = "hello world\n";
    tc.expected = "hello world\n";
    tc.position = 0;
    p.test_cases = {tc};

    int pid = ProblemMapper::insert(p);
    ASSERT_GT(pid, 0);

    ExecutionResult result = ExecutorService::execute(pid, kStringCode);

    ASSERT_EQ(result.total, 1);

    ProblemMapper::deleteById(pid);
}

TEST_F(ExecutorServiceTest, ResultsContainInputExpectedActual) {
    ExecutionResult result = ExecutorService::execute(problem_id_, kCorrectCode);

    ASSERT_EQ(result.status, "AC");
    ASSERT_GE(result.results.size(), 1u);

    for (const auto &r : result.results) {
        ASSERT_FALSE(r.input.empty());
        ASSERT_FALSE(r.expected.empty());
        ASSERT_FALSE(r.actual.empty());
    }

    ASSERT_EQ(result.results[0].input, "1 2\n");
    ASSERT_EQ(result.results[0].expected, "3\n");
}

TEST_F(ExecutorServiceTest, EmptyCodeReturnsCompileError) {
    ExecutionResult result = ExecutorService::execute(problem_id_, "");

    ASSERT_EQ(result.status, "CE");
}
