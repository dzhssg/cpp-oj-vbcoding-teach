#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

#include <string>

#include "service/session_service.h"
#include "utils/httplib.h"

constexpr const char *kSessionCookieName = "session_id";

std::string extractSessionId(const httplib::Request &req);

void setSessionCookie(httplib::Response &res, const std::string &sessionId);
void clearSessionCookie(httplib::Response &res);

void handleRegister(const httplib::Request &req, httplib::Response &res);
void handleLogin(const httplib::Request &req, httplib::Response &res);

bool authenticate(const httplib::Request &req, httplib::Response &res, SessionUser &user);
bool requireAdmin(const httplib::Request &req, httplib::Response &res, SessionUser &user);

#endif
