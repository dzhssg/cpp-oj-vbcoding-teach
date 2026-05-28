#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "utils/config.h"

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config::getInstance().reset();
    }

    void TearDown() override {
        Config::getInstance().reset();
        for (const auto &p : tmp_files_) {
            if (std::filesystem::exists(p)) {
                std::filesystem::remove(p);
            }
        }
        tmp_files_.clear();
    }

    std::string writeYaml(const std::string &content) {
        std::string path =
            std::filesystem::temp_directory_path() /
            ("config_test_" + std::to_string(tmp_files_.size()) + ".yaml");
        std::ofstream f(path);
        f << content;
        f.close();
        tmp_files_.push_back(path);
        return path;
    }

    std::vector<std::string> tmp_files_;
};

// ── Singleton ──────────────────────────────────────────────────────────

TEST_F(ConfigTest, GetInstanceReturnsSame) {
    ASSERT_EQ(&Config::getInstance(), &Config::getInstance());
}

// ── Default values ──────────────────────────────────────────────────────

TEST_F(ConfigTest, DefaultServerConfig) {
    const auto &s = Config::getInstance().server();
    EXPECT_EQ(s.host, "0.0.0.0");
    EXPECT_EQ(s.port, 8080);
}

TEST_F(ConfigTest, DefaultDatabaseConfig) {
    const auto &d = Config::getInstance().database();
    EXPECT_EQ(d.host, "127.0.0.1");
    EXPECT_EQ(d.port, 3306);
    EXPECT_EQ(d.user, "root");
    EXPECT_EQ(d.password, "");
    EXPECT_EQ(d.name, "cpp_oj");
    EXPECT_EQ(d.pool_size, 4);
}

TEST_F(ConfigTest, DefaultLoggerConfig) {
    const auto &l = Config::getInstance().logger();
    EXPECT_EQ(l.level, "INFO");
    EXPECT_EQ(l.file, "logs/oj.log");
}

// ── Load full YAML ──────────────────────────────────────────────────────

TEST_F(ConfigTest, LoadAllSectionsFromYaml) {
    std::string yaml =
        "server:\n"
        "  host: \"192.168.1.1\"\n"
        "  port: 9090\n"
        "database:\n"
        "  host: \"10.0.0.1\"\n"
        "  port: 3307\n"
        "  user: \"admin\"\n"
        "  password: \"secret\"\n"
        "  name: \"my_oj\"\n"
        "  pool_size: 8\n"
        "logger:\n"
        "  level: \"DEBUG\"\n"
        "  file: \"/var/log/oj.log\"\n";

    std::string path = writeYaml(yaml);
    ASSERT_TRUE(Config::getInstance().load(path));

    EXPECT_EQ(Config::getInstance().server().host, "192.168.1.1");
    EXPECT_EQ(Config::getInstance().server().port, 9090);
    EXPECT_EQ(Config::getInstance().database().host, "10.0.0.1");
    EXPECT_EQ(Config::getInstance().database().port, 3307);
    EXPECT_EQ(Config::getInstance().database().user, "admin");
    EXPECT_EQ(Config::getInstance().database().password, "secret");
    EXPECT_EQ(Config::getInstance().database().name, "my_oj");
    EXPECT_EQ(Config::getInstance().database().pool_size, 8);
    EXPECT_EQ(Config::getInstance().logger().level, "DEBUG");
    EXPECT_EQ(Config::getInstance().logger().file, "/var/log/oj.log");
}

// ── Load partial YAML (missing sections keep defaults) ─────────────────

TEST_F(ConfigTest, LoadOnlyServerSection) {
    std::string yaml =
        "server:\n"
        "  host: \"172.16.0.1\"\n"
        "  port: 3000\n";

    std::string path = writeYaml(yaml);
    ASSERT_TRUE(Config::getInstance().load(path));

    EXPECT_EQ(Config::getInstance().server().host, "172.16.0.1");
    EXPECT_EQ(Config::getInstance().server().port, 3000);
    // Database should stay at defaults
    EXPECT_EQ(Config::getInstance().database().host, "127.0.0.1");
    EXPECT_EQ(Config::getInstance().database().port, 3306);
    // Logger should stay at defaults
    EXPECT_EQ(Config::getInstance().logger().level, "INFO");
}

TEST_F(ConfigTest, LoadEmptyYamlKeepsAllDefaults) {
    std::string path = writeYaml("");
    ASSERT_TRUE(Config::getInstance().load(path));

    EXPECT_EQ(Config::getInstance().server().host, "0.0.0.0");
    EXPECT_EQ(Config::getInstance().server().port, 8080);
    EXPECT_EQ(Config::getInstance().database().host, "127.0.0.1");
    EXPECT_EQ(Config::getInstance().database().port, 3306);
    EXPECT_EQ(Config::getInstance().logger().level, "INFO");
}

// ── Error cases ────────────────────────────────────────────────────────

TEST_F(ConfigTest, LoadNonExistentFileReturnsFalse) {
    EXPECT_FALSE(Config::getInstance().load("/nonexistent/config.yaml"));
}

TEST_F(ConfigTest, LoadMalformedYamlReturnsFalse) {
    std::string path = writeYaml("key: [unclosed");
    EXPECT_FALSE(Config::getInstance().load(path));
}

TEST_F(ConfigTest, LoadFileWithOnlyCommentsKeepsDefaults) {
    std::string path = writeYaml("# Just a comment\n# Another line\n");
    ASSERT_TRUE(Config::getInstance().load(path));
    EXPECT_EQ(Config::getInstance().server().host, "0.0.0.0");
}

// ── Re-load overwrites previously loaded values ────────────────────────

TEST_F(ConfigTest, SecondLoadOverwritesPrevious) {
    std::string yaml1 =
        "server:\n"
        "  port: 1111\n"
        "logger:\n"
        "  level: \"DEBUG\"\n";
    std::string yaml2 =
        "server:\n"
        "  port: 2222\n"
        "database:\n"
        "  name: \"second_db\"\n";

    std::string path1 = writeYaml(yaml1);
    std::string path2 = writeYaml(yaml2);

    ASSERT_TRUE(Config::getInstance().load(path1));
    EXPECT_EQ(Config::getInstance().server().port, 1111);
    EXPECT_EQ(Config::getInstance().logger().level, "DEBUG");
    EXPECT_EQ(Config::getInstance().database().name, "cpp_oj"); // default

    ASSERT_TRUE(Config::getInstance().load(path2));
    EXPECT_EQ(Config::getInstance().server().port, 2222);
    // logger section not in yaml2 → previous value persists (no reset)
    EXPECT_EQ(Config::getInstance().logger().level, "DEBUG");
    EXPECT_EQ(Config::getInstance().database().name, "second_db");
}

// ── Producing the real config.yaml ─────────────────────────────────────

TEST_F(ConfigTest, LoadRealConfigFile) {
    std::string real_path = "../../config/config.yaml";
    if (std::filesystem::exists(real_path)) {
        ASSERT_TRUE(Config::getInstance().load(real_path));
        EXPECT_EQ(Config::getInstance().database().name, "cpp_oj");
    } else {
        GTEST_SKIP() << "config/config.yaml not found relative to test cwd";
    }
}
