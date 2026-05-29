#ifndef PROBLEM_HANDLER_H
#define PROBLEM_HANDLER_H

#include "utils/httplib.h"

void handleGetProblems(const httplib::Request &req, httplib::Response &res);
void handleGetProblemById(const httplib::Request &req, httplib::Response &res);

#endif
