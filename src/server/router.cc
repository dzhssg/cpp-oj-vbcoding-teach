#include "router.h"

#include "handler/admin_handler.h"
#include "handler/auth_handler.h"
#include "handler/problem_handler.h"
#include "handler/submit_handler.h"

void registerRoutes(httplib::Server &server) {
    // User-facing problem APIs
    server.Get("/api/problems", handleGetProblems);
    server.Get(R"(/api/problems/(\d+))", handleGetProblemById);

    // Code submission
    server.Post("/api/submit", handleSubmit);

    // Auth APIs
    server.Post("/api/register", handleRegister);
    server.Post("/api/login", handleLogin);

    // Admin problem APIs
    server.Post("/api/admin/problems", handleAdminCreateProblem);
    server.Delete(R"(/api/admin/problems/(\d+))", handleAdminDeleteProblem);
}
