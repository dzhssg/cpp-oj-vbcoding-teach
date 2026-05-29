#ifndef ADMIN_HANDLER_H
#define ADMIN_HANDLER_H

#include "utils/httplib.h"

void handleAdminCreateProblem(const httplib::Request &req, httplib::Response &res);
void handleAdminDeleteProblem(const httplib::Request &req, httplib::Response &res);

#endif
