#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>

#include "db/connection_pool.h"
#include "model/problem.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

class ProblemMapperTest : public ::testing::Test {
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
        p.code_template = "#include <iostream>\nint main() { return 0; }";
        int id = ProblemMapper::insert(p);
        if (id > 0) created_ids_.push_back(id);
        return id;
    }

    std::vector<int> created_ids_;
};

}  // namespace

// ── Insert ────────────────────────────────────────────────────────────────

TEST_F(ProblemMapperTest, InsertReturnsPositiveId) {
    int id = createProblem("Two Sum", "Easy", "Given an array...");
    ASSERT_GT(id, 0);
}

TEST_F(ProblemMapperTest, InsertMultipleReturnsDistinctIds) {
    int id1 = createProblem("P1");
    int id2 = createProblem("P2");
    int id3 = createProblem("P3");
    ASSERT_GT(id1, 0);
    ASSERT_GT(id2, 0);
    ASSERT_GT(id3, 0);
    ASSERT_NE(id1, id2);
    ASSERT_NE(id2, id3);
    ASSERT_NE(id1, id3);
}

TEST_F(ProblemMapperTest, InsertWithSpecialCharacters) {
    Problem p;
    p.title = "What's \"this\"?";
    p.difficulty = "Hard";
    p.content = "Line 1\nLine 2\nIt's a test";
    int id = ProblemMapper::insert(p);
    ASSERT_GT(id, 0);
    created_ids_.push_back(id);

    Problem found = ProblemMapper::findById(id);
    ASSERT_EQ(found.title, "What's \"this\"?");
    ASSERT_EQ(found.content, "Line 1\nLine 2\nIt's a test");
}

TEST_F(ProblemMapperTest, InsertWithTestCaseArray) {
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

    int id = ProblemMapper::insert(p);
    ASSERT_GT(id, 0);
    created_ids_.push_back(id);

    Problem found = ProblemMapper::findById(id);
    ASSERT_EQ(found.test_cases.size(), 2u);
    ASSERT_EQ(found.test_cases[0].input, "1 2");
    ASSERT_EQ(found.test_cases[0].expected, "3");
    ASSERT_EQ(found.test_cases[1].input, "10 20");
    ASSERT_EQ(found.test_cases[1].expected, "30");
}

// ── FindById ──────────────────────────────────────────────────────────────

TEST_F(ProblemMapperTest, FindByIdReturnsCorrectData) {
    int id = createProblem("Quick Sort", "Medium", "Sort the array");
    ASSERT_GT(id, 0);

    Problem p = ProblemMapper::findById(id);
    ASSERT_EQ(p.id, id);
    ASSERT_EQ(p.title, "Quick Sort");
    ASSERT_EQ(p.difficulty, "Medium");
    ASSERT_EQ(p.content, "Sort the array");
    ASSERT_NE(p.code_template.find("#include"), std::string::npos);
}

TEST_F(ProblemMapperTest, FindByIdNotFoundReturnsZeroId) {
    Problem p = ProblemMapper::findById(999999);
    ASSERT_EQ(p.id, 0);
}

TEST_F(ProblemMapperTest, FindByIdEmptyCodeTemplate) {
    Problem p;
    p.title = "No Template";
    p.difficulty = "Easy";
    p.content = "No template given";
    int id = ProblemMapper::insert(p);
    ASSERT_GT(id, 0);
    created_ids_.push_back(id);

    Problem found = ProblemMapper::findById(id);
    ASSERT_EQ(found.code_template, "");
}

// ── FindAll ───────────────────────────────────────────────────────────────

TEST_F(ProblemMapperTest, FindAllReturnsAllProblems) {
    int id1 = createProblem("Problem A");
    int id2 = createProblem("Problem B");
    int id3 = createProblem("Problem C");

    auto problems = ProblemMapper::findAll();
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

TEST_F(ProblemMapperTest, FindAllDoesNotIncludeTestCases) {
    Problem p;
    p.title = "List Check";
    p.difficulty = "Easy";
    p.content = "Content";
    p.test_cases = {{0, 0, "1", "1", 0}};
    int id = ProblemMapper::insert(p);
    ASSERT_GT(id, 0);
    created_ids_.push_back(id);

    auto problems = ProblemMapper::findAll();
    ASSERT_GE(problems.size(), 1u);
    for (const auto &pr : problems) {
        ASSERT_TRUE(pr.test_cases.empty());
    }
}

// ── DeleteById ────────────────────────────────────────────────────────────

TEST_F(ProblemMapperTest, DeleteRemovesProblem) {
    int id = createProblem("To Delete");
    ASSERT_GT(id, 0);

    bool deleted = ProblemMapper::deleteById(id);
    ASSERT_TRUE(deleted);

    Problem p = ProblemMapper::findById(id);
    ASSERT_EQ(p.id, 0);
}

TEST_F(ProblemMapperTest, DeleteNonExistentReturnsFalse) {
    bool deleted = ProblemMapper::deleteById(999999);
    ASSERT_FALSE(deleted);
}

TEST_F(ProblemMapperTest, DeleteCascadesToTestCases) {
    Problem p;
    p.title = "Cascade Test";
    p.difficulty = "Easy";
    p.content = "Test cascade";
    p.test_cases = {{0, 0, "in", "out", 0}};
    int id = ProblemMapper::insert(p);
    ASSERT_GT(id, 0);

    Problem before = ProblemMapper::findById(id);
    ASSERT_EQ(before.test_cases.size(), 1u);

    ProblemMapper::deleteById(id);

    Problem after = ProblemMapper::findById(id);
    ASSERT_EQ(after.id, 0);
    ASSERT_EQ(after.test_cases.size(), 0u);
}

// ── Edge cases ────────────────────────────────────────────────────────────

TEST_F(ProblemMapperTest, InsertLongContent) {
    std::string long_content(5000, 'x');
    Problem p;
    p.title = "Long Content";
    p.difficulty = "Medium";
    p.content = long_content;
    int id = ProblemMapper::insert(p);
    ASSERT_GT(id, 0);
    created_ids_.push_back(id);

    Problem found = ProblemMapper::findById(id);
    ASSERT_EQ(found.content, long_content);
}

TEST_F(ProblemMapperTest, DifficultyCasePreserved) {
    Problem p;
    p.title = "Difficult Problem";
    p.difficulty = "Hard";
    p.content = "Very hard indeed";
    int id = ProblemMapper::insert(p);
    ASSERT_GT(id, 0);
    created_ids_.push_back(id);

    Problem found = ProblemMapper::findById(id);
    ASSERT_EQ(found.difficulty, "Hard");
}
