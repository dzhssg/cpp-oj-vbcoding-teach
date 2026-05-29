#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "db/connection_pool.h"
#include "model/problem.h"
#include "model/test_case.h"
#include "service/problem_service.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

class ProblemServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, 2);
    }

    void TearDown() override {
        for (int id : created_ids_) {
            ProblemMapper::deleteById(id);
        }
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    int createProblem(const std::string &title,
                      const std::string &difficulty = "Easy",
                      const std::string &content = "test content") {
        Problem p;
        p.title = title;
        p.difficulty = difficulty;
        p.content = content;
        int id = ProblemService::createProblem(p);
        if (id > 0) created_ids_.push_back(id);
        return id;
    }

    std::vector<int> created_ids_;
};

}  // namespace

TEST_F(ProblemServiceTest, CreateProblemReturnsPositiveId) {
    int id = createProblem("Test Problem", "Easy", "Some content");
    ASSERT_GT(id, 0);
}

TEST_F(ProblemServiceTest, CreateProblemWithEmptyTitleReturnsError) {
    Problem p;
    p.title = "";
    p.difficulty = "Easy";
    p.content = "content";
    int id = ProblemService::createProblem(p);
    ASSERT_EQ(id, -1);
}

TEST_F(ProblemServiceTest, CreateProblemWithEmptyContentReturnsError) {
    Problem p;
    p.title = "Valid Title";
    p.difficulty = "Easy";
    p.content = "";
    int id = ProblemService::createProblem(p);
    ASSERT_EQ(id, -1);
}

TEST_F(ProblemServiceTest, CreateProblemWithTestCases) {
    Problem p;
    p.title = "A+B Problem";
    p.difficulty = "Easy";
    p.content = "Sum two numbers";

    TestCase tc1;
    tc1.input = "1 2";
    tc1.expected = "3";
    tc1.position = 0;

    TestCase tc2;
    tc2.input = "10 20";
    tc2.expected = "30";
    tc2.position = 1;

    p.test_cases = {tc1, tc2};

    int id = ProblemService::createProblem(p);
    ASSERT_GT(id, 0);
    created_ids_.push_back(id);

    Problem found = ProblemService::getProblemById(id);
    ASSERT_EQ(found.test_cases.size(), 2u);
    ASSERT_EQ(found.test_cases[0].input, "1 2");
    ASSERT_EQ(found.test_cases[0].expected, "3");
}

TEST_F(ProblemServiceTest, GetAllProblemsReturnsList) {
    int id1 = createProblem("Problem A");
    int id2 = createProblem("Problem B");
    int id3 = createProblem("Problem C");

    auto problems = ProblemService::getAllProblems();
    ASSERT_GE(problems.size(), 3u);

    bool found_a = false, found_b = false, found_c = false;
    for (const auto &p : problems) {
        if (p.title == "Problem A") found_a = true;
        if (p.title == "Problem B") found_b = true;
        if (p.title == "Problem C") found_c = true;
    }
    ASSERT_TRUE(found_a);
    ASSERT_TRUE(found_b);
    ASSERT_TRUE(found_c);
}

TEST_F(ProblemServiceTest, GetProblemByIdReturnsCorrectData) {
    int id = createProblem("Binary Search", "Medium", "Find target in sorted array");
    ASSERT_GT(id, 0);

    Problem p = ProblemService::getProblemById(id);
    ASSERT_EQ(p.id, id);
    ASSERT_EQ(p.title, "Binary Search");
    ASSERT_EQ(p.difficulty, "Medium");
    ASSERT_EQ(p.content, "Find target in sorted array");
}

TEST_F(ProblemServiceTest, GetProblemByIdNotFoundReturnsZeroId) {
    Problem p = ProblemService::getProblemById(999999);
    ASSERT_EQ(p.id, 0);
}

TEST_F(ProblemServiceTest, DeleteProblemRemovesIt) {
    int id = createProblem("To Be Deleted");
    ASSERT_GT(id, 0);

    bool deleted = ProblemService::deleteProblemById(id);
    ASSERT_TRUE(deleted);

    Problem p = ProblemService::getProblemById(id);
    ASSERT_EQ(p.id, 0);
}

TEST_F(ProblemServiceTest, DeleteNonExistentProblemReturnsFalse) {
    bool deleted = ProblemService::deleteProblemById(999999);
    ASSERT_FALSE(deleted);
}

TEST_F(ProblemServiceTest, DeleteCascadesToTestCases) {
    Problem p;
    p.title = "Cascade Delete";
    p.difficulty = "Easy";
    p.content = "Cascade test";
    p.test_cases = {{0, 0, "input", "output", 0}};
    int id = ProblemService::createProblem(p);
    ASSERT_GT(id, 0);

    Problem before = ProblemService::getProblemById(id);
    ASSERT_EQ(before.test_cases.size(), 1u);

    ProblemService::deleteProblemById(id);

    Problem after = ProblemService::getProblemById(id);
    ASSERT_EQ(after.id, 0);
}

TEST_F(ProblemServiceTest, CreateDuplicateTitleAllowed) {
    int id1 = createProblem("Same Title");
    int id2 = createProblem("Same Title");
    ASSERT_GT(id1, 0);
    ASSERT_GT(id2, 0);
    ASSERT_NE(id1, id2);
}
