#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include "utils/logger.h"

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().shutdown();
        old_cout_ = std::cout.rdbuf(out_buf_.rdbuf());
        old_cerr_ = std::cerr.rdbuf(err_buf_.rdbuf());
    }

    void TearDown() override {
        std::cout.rdbuf(old_cout_);
        std::cerr.rdbuf(old_cerr_);
        Logger::getInstance().shutdown();
        if (!tmp_file_.empty() && std::filesystem::exists(tmp_file_)) {
            std::filesystem::remove(tmp_file_);
        }
    }

    std::string coutStr() { return out_buf_.str(); }
    std::string cerrStr() { return err_buf_.str(); }

    void initLoggerWithFile() {
        tmp_file_ = std::filesystem::temp_directory_path() / "logger_test.log";
        Logger::getInstance().init(LogLevel::DEBUG, tmp_file_);
    }

    std::string readLogFile() {
        std::ifstream f(tmp_file_);
        std::string content((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
        return content;
    }

    std::stringstream out_buf_;
    std::stringstream err_buf_;
    std::streambuf *old_cout_ = nullptr;
    std::streambuf *old_cerr_ = nullptr;
    std::string tmp_file_;
};

// ── Singleton ──────────────────────────────────────────────────────────

TEST_F(LoggerTest, GetInstanceReturnsSame) {
    Logger &a = Logger::getInstance();
    Logger &b = Logger::getInstance();
    ASSERT_EQ(&a, &b);
}

// ── Level string conversion ─────────────────────────────────────────────

TEST_F(LoggerTest, LevelFromStringDebug) {
    ASSERT_EQ(Logger::levelFromString("DEBUG"), LogLevel::DEBUG);
}

TEST_F(LoggerTest, LevelFromStringInfo) {
    ASSERT_EQ(Logger::levelFromString("INFO"), LogLevel::INFO);
}

TEST_F(LoggerTest, LevelFromStringWarning) {
    ASSERT_EQ(Logger::levelFromString("WARNING"), LogLevel::WARNING);
}

TEST_F(LoggerTest, LevelFromStringError) {
    ASSERT_EQ(Logger::levelFromString("ERROR"), LogLevel::ERROR);
}

TEST_F(LoggerTest, LevelFromStringInvalidDefaultsToInfo) {
    ASSERT_EQ(Logger::levelFromString("TRACE"), LogLevel::INFO);
    ASSERT_EQ(Logger::levelFromString(""), LogLevel::INFO);
}

// ── setLevel ────────────────────────────────────────────────────────────

TEST_F(LoggerTest, SetLevelBelowSuppressesMessage) {
    Logger::getInstance().setLevel(LogLevel::WARNING);
    Logger::getInstance().info("should not appear");
    Logger::getInstance().debug("should not appear either");

    EXPECT_TRUE(coutStr().empty());
}

TEST_F(LoggerTest, SetLevelAllowsHigherOrEqual) {
    Logger::getInstance().setLevel(LogLevel::WARNING);
    Logger::getInstance().warning("warn msg");
    Logger::getInstance().error("err msg");

    EXPECT_NE(cerrStr().find("warn msg"), std::string::npos);
    EXPECT_NE(cerrStr().find("err msg"), std::string::npos);
}

// ── Output destination ──────────────────────────────────────────────────

TEST_F(LoggerTest, InfoGoesToStdout) {
    Logger::getInstance().init(LogLevel::DEBUG, "");
    Logger::getInstance().info("hello stdout");

    EXPECT_NE(coutStr().find("hello stdout"), std::string::npos);
    EXPECT_NE(coutStr().find("[INFO]"), std::string::npos);
}

TEST_F(LoggerTest, DebugGoesToStdout) {
    Logger::getInstance().init(LogLevel::DEBUG, "");
    Logger::getInstance().debug("debug message");

    EXPECT_NE(coutStr().find("debug message"), std::string::npos);
    EXPECT_NE(coutStr().find("[DEBUG]"), std::string::npos);
}

TEST_F(LoggerTest, WarningGoesToStderr) {
    Logger::getInstance().init(LogLevel::INFO, "");
    Logger::getInstance().warning("warn output");

    EXPECT_NE(cerrStr().find("warn output"), std::string::npos);
    EXPECT_NE(cerrStr().find("[WARN]"), std::string::npos);
}

TEST_F(LoggerTest, ErrorGoesToStderr) {
    Logger::getInstance().init(LogLevel::INFO, "");
    Logger::getInstance().error("error output");

    EXPECT_NE(cerrStr().find("error output"), std::string::npos);
    EXPECT_NE(cerrStr().find("[ERROR]"), std::string::npos);
}

// ── Timestamp format ────────────────────────────────────────────────────

TEST_F(LoggerTest, OutputContainsTimestamp) {
    Logger::getInstance().init(LogLevel::INFO, "");
    Logger::getInstance().info("ts check");

    std::string out = coutStr();
    // Expect something like [YYYY-MM-DD HH:MM:SS.mmm] [INFO] ts check
    ASSERT_GE(out.size(), 20u);
    EXPECT_EQ(out[0], '[');  // starts with timestamp bracket
    EXPECT_NE(out.find("] [INFO] ts check\n"), std::string::npos);
}

// ── File output ─────────────────────────────────────────────────────────

TEST_F(LoggerTest, LogWritesToFile) {
    initLoggerWithFile();
    Logger::getInstance().info("file line");

    // Flush by shutting down (TearDown calls shutdown, which closes file)
    Logger::getInstance().shutdown();

    std::string content = readLogFile();
    EXPECT_NE(content.find("file line"), std::string::npos);
    EXPECT_NE(content.find("[INFO]"), std::string::npos);
}

TEST_F(LoggerTest, EmptyFilePathDoesNotCrash) {
    Logger::getInstance().init(LogLevel::INFO, "");
    Logger::getInstance().info("no file, no crash");
    EXPECT_NE(coutStr().find("no file, no crash"), std::string::npos);
}

// ── Macros ──────────────────────────────────────────────────────────────

TEST_F(LoggerTest, MacroInfoWorks) {
    Logger::getInstance().init(LogLevel::INFO, "");
    LOG_INFO("macro info");
    EXPECT_NE(coutStr().find("macro info"), std::string::npos);
}

TEST_F(LoggerTest, MacroErrorWorks) {
    Logger::getInstance().init(LogLevel::INFO, "");
    LOG_ERROR("macro error");
    EXPECT_NE(cerrStr().find("macro error"), std::string::npos);
}

TEST_F(LoggerTest, MacroDebugSuppressedAtInfoLevel) {
    Logger::getInstance().init(LogLevel::INFO, "");
    LOG_DEBUG("macro debug");
    EXPECT_EQ(coutStr().find("macro debug"), std::string::npos);
}

// ── Thread safety ───────────────────────────────────────────────────────

TEST_F(LoggerTest, MultipleThreadsLogConcurrently) {
    Logger::getInstance().init(LogLevel::INFO, "");

    std::thread t1([]() {
        for (int i = 0; i < 50; ++i) {
            Logger::getInstance().info("thread 1 msg " + std::to_string(i));
        }
    });
    std::thread t2([]() {
        for (int i = 0; i < 50; ++i) {
            Logger::getInstance().warning("thread 2 msg " + std::to_string(i));
        }
    });
    std::thread t3([]() {
        for (int i = 0; i < 50; ++i) {
            Logger::getInstance().error("thread 3 msg " + std::to_string(i));
        }
    });

    t1.join();
    t2.join();
    t3.join();

    // All 150 lines should appear (no data race that drops output)
    std::string out = coutStr();
    std::string err = cerrStr();

    int count = 0;
    std::istringstream iss(out + err);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) ++count;
    }
    EXPECT_EQ(count, 150);
}

// ── shutdown resets state ───────────────────────────────────────────────

TEST_F(LoggerTest, ShutdownResetsLogLevel) {
    Logger::getInstance().setLevel(LogLevel::ERROR);
    Logger::getInstance().shutdown();
    Logger::getInstance().info("after shutdown");
    // After shutdown, level resets to INFO, so INFO should appear
    EXPECT_NE(coutStr().find("after shutdown"), std::string::npos);
}

TEST_F(LoggerTest, ShutdownClosesFile) {
    initLoggerWithFile();
    Logger::getInstance().info("before close");
    Logger::getInstance().shutdown();

    // File should exist with the logged content
    EXPECT_TRUE(std::filesystem::exists(tmp_file_));
    std::string content = readLogFile();
    EXPECT_NE(content.find("before close"), std::string::npos);

    // After shutdown, writing should not append (file is closed)
    Logger::getInstance().info("after close - should be lost");
    std::string content2 = readLogFile();
    EXPECT_EQ(content2.find("after close"), std::string::npos);
}
