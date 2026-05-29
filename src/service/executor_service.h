#ifndef EXECUTOR_SERVICE_H
#define EXECUTOR_SERVICE_H

#include <string>
#include <vector>

struct TestCaseResult {
    int position;
    std::string status;
    std::string input;
    std::string expected;
    std::string actual;
    std::string error;
};

struct ExecutionResult {
    std::string status;
    std::string compile_error;
    std::vector<TestCaseResult> results;
    int passed = 0;
    int total = 0;
};

class ExecutorService {
public:
    ExecutorService() = delete;

    static ExecutionResult execute(int problemId, const std::string &code);

private:
    static bool compile(const std::string &srcPath,
                        const std::string &binPath,
                        std::string &error);
    static TestCaseResult runTestCase(const std::string &binPath,
                                      const std::string &input,
                                      const std::string &expected,
                                      int position,
                                      int timeoutSec,
                                      int memoryLimitMb);
    static std::string makeTempDir();
    static bool writeFile(const std::string &path, const std::string &content);
};

#endif
