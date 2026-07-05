#ifndef BCRYPT_H
#define BCRYPT_H

#include <string>

std::string bcryptHash(const std::string &password, unsigned int cost = 10);
bool bcryptVerify(const std::string &password, const std::string &hash);

#endif
