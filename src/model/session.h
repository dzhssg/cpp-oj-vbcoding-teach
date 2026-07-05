#ifndef SESSION_H
#define SESSION_H

#include <string>

struct Session {
    std::string id;
    int user_id = 0;
    std::string expires_at;
    std::string created_at;
};

class SessionMapper {
public:
    SessionMapper() = delete;

    static bool insert(const Session &session);
    static Session findById(const std::string &id);
    static bool deleteById(const std::string &id);
    static int deleteExpired();
};

#endif
