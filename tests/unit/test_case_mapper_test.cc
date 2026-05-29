#include <gtest/gtest.h>

#include "db/connection_pool.h"
#include "model/problem.h"
#include "model/test_case.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";

class TestCaseMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, 2);

        Problem p;
        p.title = "TestCase Mapper Problem";
        p.difficulty = "Easy";
        p.content = "For testing test cases";
        problem_id_ = ProblemMapper::insert(p);
        ASSERT_GT(problem_id_, 0);
    }

    void TearDown() override {
        ProblemMapper::deleteById(problem_id_);
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    int problem_id_ = 0;
};

}  // namespace

// ── Insert ────────────────────────────────────────────────────────────────

TEST_F(TestCaseMapperTest, InsertReturnsPositiveId) {
    TestCase tc;
    tc.problem_id = problem_id_;
    tc.input = "1 2";
    tc.expected = "3";
    tc.position = 0;
    int id = TestCaseMapper::insert(tc);
    ASSERT_GT(id, 0);
}

TEST_F(TestCaseMapperTest, InsertMultipleReturnsDistinctIds) {
    TestCase tc1{0, problem_id_, "1", "1", 0};
    TestCase tc2{0, problem_id_, "2", "2", 1};

    int id1 = TestCaseMapper::insert(tc1);
    int id2 = TestCaseMapper::insert(tc2);
    ASSERT_GT(id1, 0);
    ASSERT_GT(id2, 0);
    ASSERT_NE(id1, id2);
}

TEST_F(TestCaseMapperTest, InsertSpecialCharacters) {
    TestCase tc;
    tc.problem_id = problem_id_;
    tc.input = "hello 'world'\n\"quoted\"";
    tc.expected = "result\\backslash";
    tc.position = 0;
    int id = TestCaseMapper::insert(tc);
    ASSERT_GT(id, 0);

    auto cases = TestCaseMapper::findByProblemId(problem_id_);
    ASSERT_EQ(cases.size(), 1u);
    ASSERT_EQ(cases[0].input, "hello 'world'\n\"quoted\"");
    ASSERT_EQ(cases[0].expected, "result\\backslash");
}

// ── FindByProblemId ───────────────────────────────────────────────────────

TEST_F(TestCaseMapperTest, FindByProblemIdReturnsSorted) {
    TestCase tc1{0, problem_id_, "input2", "output2", 2};
    TestCase tc2{0, problem_id_, "input0", "output0", 0};
    TestCase tc3{0, problem_id_, "input1", "output1", 1};

    TestCaseMapper::insert(tc1);
    TestCaseMapper::insert(tc2);
    TestCaseMapper::insert(tc3);

    auto cases = TestCaseMapper::findByProblemId(problem_id_);
    ASSERT_EQ(cases.size(), 3u);
    ASSERT_EQ(cases[0].position, 0);
    ASSERT_EQ(cases[1].position, 1);
    ASSERT_EQ(cases[2].position, 2);
    ASSERT_EQ(cases[0].input, "input0");
    ASSERT_EQ(cases[1].input, "input1");
    ASSERT_EQ(cases[2].input, "input2");
}

TEST_F(TestCaseMapperTest, FindByProblemIdNotFoundReturnsEmpty) {
    auto cases = TestCaseMapper::findByProblemId(999999);
    ASSERT_TRUE(cases.empty());
}

TEST_F(TestCaseMapperTest, FindByProblemIdMultipleTestCases) {
    for (int i = 0; i < 5; ++i) {
        TestCase tc{0, problem_id_,
                    "in" + std::to_string(i),
                    "out" + std::to_string(i), i};
        TestCaseMapper::insert(tc);
    }
    auto cases = TestCaseMapper::findByProblemId(problem_id_);
    ASSERT_EQ(cases.size(), 5u);
}

// ── DeleteByProblemId ─────────────────────────────────────────────────────

TEST_F(TestCaseMapperTest, DeleteByProblemIdRemovesAll) {
    TestCaseMapper::insert({0, problem_id_, "a", "b", 0});
    TestCaseMapper::insert({0, problem_id_, "c", "d", 1});

    auto before = TestCaseMapper::findByProblemId(problem_id_);
    ASSERT_EQ(before.size(), 2u);

    bool deleted = TestCaseMapper::deleteByProblemId(problem_id_);
    ASSERT_TRUE(deleted);

    auto after = TestCaseMapper::findByProblemId(problem_id_);
    ASSERT_TRUE(after.empty());
}

TEST_F(TestCaseMapperTest, DeleteByProblemIdNonExistentReturnsFalse) {
    bool deleted = TestCaseMapper::deleteByProblemId(999999);
    ASSERT_FALSE(deleted);
}

// ── Position ordering ─────────────────────────────────────────────────────

TEST_F(TestCaseMapperTest, SamePositionOrderedById) {
    TestCase tc1{0, problem_id_, "first", "1", 0};
    int id1 = TestCaseMapper::insert(tc1);
    TestCase tc2{0, problem_id_, "second", "2", 0};
    int id2 = TestCaseMapper::insert(tc2);

    auto cases = TestCaseMapper::findByProblemId(problem_id_);
    ASSERT_EQ(cases.size(), 2u);
    ASSERT_LT(cases[0].id, cases[1].id);
}
