#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "db/connection_pool.h"
#include "utils/logger.h"

namespace {

constexpr const char *kHost = "127.0.0.1";
constexpr int kPort = 3306;
constexpr const char *kUser = "root";
constexpr const char *kPass = "";
constexpr const char *kDb = "cpp_oj";
constexpr int kPoolSize = 4;

class ConnectionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::ERROR);
        ConnectionPool::getInstance().shutdown();
        old_cout_ = std::cout.rdbuf(null_buf_.rdbuf());
        old_cerr_ = std::cerr.rdbuf(null_buf_.rdbuf());
    }

    void TearDown() override {
        std::cout.rdbuf(old_cout_);
        std::cerr.rdbuf(old_cerr_);
        ConnectionPool::getInstance().shutdown();
        Logger::getInstance().setLevel(LogLevel::INFO);
    }

    void initPool(int size = kPoolSize) {
        ConnectionPool::getInstance().init(kHost, kPort, kUser, kPass, kDb, size);
    }

    std::stringstream null_buf_;
    std::streambuf *old_cout_ = nullptr;
    std::streambuf *old_cerr_ = nullptr;
};

}  // namespace

// ── Singleton ────────────────────────────────────────────────────────────

TEST_F(ConnectionPoolTest, GetInstanceReturnsSame) {
    ConnectionPool &a = ConnectionPool::getInstance();
    ConnectionPool &b = ConnectionPool::getInstance();
    ASSERT_EQ(&a, &b);
}

// ── Uninitialized pool behaviour ─────────────────────────────────────────

TEST_F(ConnectionPoolTest, AcquireBeforeInitReturnsNull) {
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_EQ(conn, nullptr);
}

TEST_F(ConnectionPoolTest, ReleaseNullDoesNotCrash) {
    ConnectionPool::getInstance().release(nullptr);
    SUCCEED();
}

TEST_F(ConnectionPoolTest, ShutdownBeforeInitDoesNotCrash) {
    ConnectionPool::getInstance().shutdown();
    SUCCEED();
}

TEST_F(ConnectionPoolTest, DoubleShutdownDoesNotCrash) {
    ConnectionPool::getInstance().shutdown();
    ConnectionPool::getInstance().shutdown();
    SUCCEED();
}

// ── Normal lifecycle ─────────────────────────────────────────────────────

TEST_F(ConnectionPoolTest, InitCreatesConnections) {
    initPool();
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_NE(conn, nullptr);
    ASSERT_EQ(mysql_ping(conn), 0);
    ConnectionPool::getInstance().release(conn);
}

TEST_F(ConnectionPoolTest, AcquireConsumesFromPool) {
    initPool(3);
    MYSQL *c1 = ConnectionPool::getInstance().acquire();
    MYSQL *c2 = ConnectionPool::getInstance().acquire();
    MYSQL *c3 = ConnectionPool::getInstance().acquire();
    ASSERT_NE(c1, nullptr);
    ASSERT_NE(c2, nullptr);
    ASSERT_NE(c3, nullptr);
    ConnectionPool::getInstance().release(c1);
    ConnectionPool::getInstance().release(c2);
    ConnectionPool::getInstance().release(c3);
}

TEST_F(ConnectionPoolTest, ReleaseAndReacquireSameConnection) {
    initPool(1);
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_NE(conn, nullptr);
    ConnectionPool::getInstance().release(conn);

    MYSQL *conn2 = ConnectionPool::getInstance().acquire();
    ASSERT_EQ(conn, conn2);
    ConnectionPool::getInstance().release(conn2);
}

TEST_F(ConnectionPoolTest, AcquireReleaseCycleMultipleTimes) {
    initPool(2);
    for (int i = 0; i < 10; ++i) {
        MYSQL *c1 = ConnectionPool::getInstance().acquire();
        MYSQL *c2 = ConnectionPool::getInstance().acquire();
        ASSERT_NE(c1, nullptr);
        ASSERT_NE(c2, nullptr);
        ASSERT_EQ(mysql_ping(c1), 0);
        ASSERT_EQ(mysql_ping(c2), 0);
        ConnectionPool::getInstance().release(c1);
        ConnectionPool::getInstance().release(c2);
    }
}

// ── Pool exhaustion and blocking ─────────────────────────────────────────

TEST_F(ConnectionPoolTest, PoolBlockedAcquireWakesOnRelease) {
    initPool(1);
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_NE(conn, nullptr);

    std::atomic<bool> acquired{false};

    std::thread t([&acquired]() {
        MYSQL *c = ConnectionPool::getInstance().acquire();
        ASSERT_NE(c, nullptr);
        acquired.store(true);
        ConnectionPool::getInstance().release(c);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(acquired.load());

    ConnectionPool::getInstance().release(conn);

    t.join();
    EXPECT_TRUE(acquired.load());
}

TEST_F(ConnectionPoolTest, MultipleBlockedAcquirersWakeOnReleases) {
    constexpr int kSize = 2;
    initPool(kSize);
    std::vector<MYSQL *> held;

    for (int i = 0; i < kSize; ++i) {
        MYSQL *conn = ConnectionPool::getInstance().acquire();
        ASSERT_NE(conn, nullptr);
        held.push_back(conn);
    }

    std::atomic<int> woken{0};
    std::vector<std::thread> waiters;
    for (int i = 0; i < 3; ++i) {
        waiters.emplace_back([&woken]() {
            MYSQL *c = ConnectionPool::getInstance().acquire();
            ASSERT_NE(c, nullptr);
            woken.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ConnectionPool::getInstance().release(c);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(woken.load(), 0);

    for (auto *c : held) {
        ConnectionPool::getInstance().release(c);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (auto &t : waiters) {
        t.join();
    }
    ASSERT_EQ(woken.load(), 3);
}

// ── ScopedConnection RAII ────────────────────────────────────────────────

TEST_F(ConnectionPoolTest, ScopedConnectionAcquiresOnConstruction) {
    initPool(1);
    ScopedConnection sc;
    ASSERT_NE(sc.get(), nullptr);
    ASSERT_EQ(mysql_ping(sc.get()), 0);
}

TEST_F(ConnectionPoolTest, ScopedConnectionReleasesOnDestruction) {
    initPool(1);
    {
        ScopedConnection sc;
        ASSERT_NE(sc.get(), nullptr);
    }
    ScopedConnection sc2;
    ASSERT_NE(sc2.get(), nullptr);
}

TEST_F(ConnectionPoolTest, ScopedConnectionFromUninitializedPoolIsNull) {
    ConnectionPool::getInstance().shutdown();
    ScopedConnection sc;
    ASSERT_EQ(sc.get(), nullptr);
}

TEST_F(ConnectionPoolTest, ScopedConnectionNotCopyable) {
    static_assert(!std::is_copy_constructible<ScopedConnection>::value,
                  "ScopedConnection should not be copy constructible");
    static_assert(!std::is_copy_assignable<ScopedConnection>::value,
                  "ScopedConnection should not be copy assignable");
    SUCCEED();
}

// ── Re-init ──────────────────────────────────────────────────────────────

TEST_F(ConnectionPoolTest, ReInitCreatesNewConnectionsDifferentSize) {
    initPool(2);
    MYSQL *old = ConnectionPool::getInstance().acquire();
    ASSERT_NE(old, nullptr);
    ConnectionPool::getInstance().release(old);

    initPool(3);

    std::vector<MYSQL *> conns;
    for (int i = 0; i < 3; ++i) {
        MYSQL *c = ConnectionPool::getInstance().acquire();
        ASSERT_NE(c, nullptr);
        conns.push_back(c);
    }
    ASSERT_EQ(conns.size(), 3u);

    for (auto *c : conns) {
        ConnectionPool::getInstance().release(c);
    }
}

// ── Shutdown ─────────────────────────────────────────────────────────────

TEST_F(ConnectionPoolTest, AcquireAfterShutdownReturnsNull) {
    initPool(1);
    ConnectionPool::getInstance().shutdown();
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_EQ(conn, nullptr);
}

TEST_F(ConnectionPoolTest, ShutdownWakesBlockedAcquirers) {
    initPool(1);
    MYSQL *conn = ConnectionPool::getInstance().acquire();

    std::atomic<bool> exited{false};

    std::thread t([&exited]() {
        MYSQL *c = ConnectionPool::getInstance().acquire();
        ASSERT_EQ(c, nullptr);
        exited.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(exited.load());

    ConnectionPool::getInstance().shutdown();

    t.join();
    ASSERT_TRUE(exited.load());
}

// ── Error paths ──────────────────────────────────────────────────────────

TEST_F(ConnectionPoolTest, InitBadHostFails) {
    ConnectionPool::getInstance().init(
        "nonexistent.host.example", 3306,
        kUser, kPass, kDb, 1);
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_EQ(conn, nullptr);
}

TEST_F(ConnectionPoolTest, InitBadPortFails) {
    ConnectionPool::getInstance().init(
        kHost, 19999,
        kUser, kPass, kDb, 1);
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_EQ(conn, nullptr);
}

TEST_F(ConnectionPoolTest, InitBadCredentialsFails) {
    ConnectionPool::getInstance().init(
        kHost, kPort,
        "no_such_user_xyz", "wrong_password_123", kDb, 1);
    MYSQL *conn = ConnectionPool::getInstance().acquire();
    ASSERT_EQ(conn, nullptr);
}

// ── Thread safety ────────────────────────────────────────────────────────

TEST_F(ConnectionPoolTest, ConcurrentAcquireRelease) {
    constexpr int kSize = 3;
    constexpr int kThreads = 6;
    constexpr int kItersPerThread = 50;
    initPool(kSize);

    std::atomic<int> total{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&total]() {
            for (int j = 0; j < kItersPerThread; ++j) {
                MYSQL *conn = ConnectionPool::getInstance().acquire();
                ASSERT_NE(conn, nullptr);
                ASSERT_EQ(mysql_ping(conn), 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                ConnectionPool::getInstance().release(conn);
                total.fetch_add(1);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    ASSERT_EQ(total.load(), kThreads * kItersPerThread);
}

TEST_F(ConnectionPoolTest, ConcurrentInitAndAcquire) {
    std::atomic<bool> start{false};
    std::atomic<int> success{0};
    std::atomic<int> failures{0};

    initPool(2);

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&start, &success, &failures]() {
            while (!start.load()) {
                std::this_thread::yield();
            }
            MYSQL *conn = ConnectionPool::getInstance().acquire();
            if (conn) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                ConnectionPool::getInstance().release(conn);
                success.fetch_add(1);
            } else {
                failures.fetch_add(1);
            }
        });
    }

    start.store(true);
    for (auto &t : threads) {
        t.join();
    }

    ASSERT_EQ(success.load(), 10);
    ASSERT_EQ(failures.load(), 0);
}
