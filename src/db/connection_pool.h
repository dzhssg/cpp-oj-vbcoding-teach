#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <mysql/mysql.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

class ConnectionPool {
public:
    static ConnectionPool &getInstance();

    void init(const std::string &host, int port,
              const std::string &user, const std::string &password,
              const std::string &dbname, int pool_size);
    void shutdown();

    MYSQL *acquire();
    void release(MYSQL *conn);

private:
    ConnectionPool() = default;
    ~ConnectionPool();
    ConnectionPool(const ConnectionPool &) = delete;
    ConnectionPool &operator=(const ConnectionPool &) = delete;

    MYSQL *createConnection();

    std::queue<MYSQL *> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;

    std::string host_;
    int port_ = 3306;
    std::string user_;
    std::string password_;
    std::string dbname_;
    int pool_size_ = 4;
    bool initialized_ = false;
};

class ScopedConnection {
public:
    ScopedConnection();
    ~ScopedConnection();

    ScopedConnection(const ScopedConnection &) = delete;
    ScopedConnection &operator=(const ScopedConnection &) = delete;

    MYSQL *get() const { return conn_; }

private:
    MYSQL *conn_ = nullptr;
};

#endif
