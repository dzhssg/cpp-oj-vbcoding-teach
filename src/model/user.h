#ifndef USER_H
#define USER_H

#include <string>

struct User {
    int id = 0;
    std::string username;
    std::string password;
    std::string role;
    std::string created_at;
};

class UserMapper {
public:
    UserMapper() = delete;

    static int insert(const User &user);
    static User findByUsername(const std::string &username);
};

#endif
