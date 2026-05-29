#include "connection_pool.h"

#include "utils/logger.h"

ConnectionPool &ConnectionPool::getInstance() {
    static ConnectionPool instance;
    return instance;
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

void ConnectionPool::init(const std::string &host, int port,
                          const std::string &user, const std::string &password,
                          const std::string &dbname, int pool_size) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        LOG_WARN("[DB POOL] Already initialized, shutting down first");
        while (!pool_.empty()) {
            MYSQL *conn = pool_.front();
            pool_.pop();
            mysql_close(conn);
        }
    }

    host_ = host;
    port_ = port;
    user_ = user;
    password_ = password;
    dbname_ = dbname;
    pool_size_ = pool_size;

    for (int i = 0; i < pool_size_; ++i) {
        MYSQL *conn = createConnection();
        if (conn) {
            pool_.push(conn);
        }
    }

    initialized_ = !pool_.empty();

    if (initialized_) {
        LOG_INFO("[DB POOL] Initialized with " + std::to_string(pool_.size())
                 + " connections (requested " + std::to_string(pool_size_) + ")");
    } else {
        LOG_ERROR("[DB POOL] Failed to create any connections");
    }
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        MYSQL *conn = pool_.front();
        pool_.pop();
        mysql_close(conn);
    }
    initialized_ = false;
    cv_.notify_all();
    LOG_INFO("[DB POOL] Shutdown complete");
}

MYSQL *ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);

    cv_.wait(lock, [this] {
        return !initialized_ || !pool_.empty();
    });

    if (!initialized_) {
        return nullptr;
    }

    MYSQL *conn = pool_.front();
    pool_.pop();

    if (mysql_ping(conn) != 0) {
        LOG_WARN("[DB POOL] Dead connection detected, recreating");
        mysql_close(conn);
        conn = createConnection();
        if (!conn) {
            LOG_ERROR("[DB POOL] Failed to recreate connection");
            return nullptr;
        }
    }

    return conn;
}

void ConnectionPool::release(MYSQL *conn) {
    if (!conn) return;

    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(conn);
    cv_.notify_one();
}

MYSQL *ConnectionPool::createConnection() {
    MYSQL *conn = mysql_init(nullptr);
    if (!conn) {
        LOG_ERROR("[DB POOL] mysql_init failed");
        return nullptr;
    }

    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    if (!mysql_real_connect(conn, host_.c_str(), user_.c_str(),
                            password_.c_str(), dbname_.c_str(), port_,
                            nullptr, 0)) {
        LOG_ERROR("[DB POOL] mysql_real_connect failed: "
                  + std::string(mysql_error(conn)));
        mysql_close(conn);
        return nullptr;
    }

    return conn;
}

ScopedConnection::ScopedConnection()
    : conn_(ConnectionPool::getInstance().acquire()) {}

ScopedConnection::~ScopedConnection() {
    if (conn_) {
        ConnectionPool::getInstance().release(conn_);
    }
}
