#include "executor_service.h"

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#include "model/problem.h"
#include "model/test_case.h"
#include "utils/logger.h"

static constexpr int kDefaultTimeoutSec = 5;
static constexpr int kDefaultMemoryLimitMb = 256;
static constexpr int kMaxOutputBytes = 65536;

static std::string readAll(int fd) {
    std::string result;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        result.append(buf, static_cast<size_t>(n));
        if (result.size() > static_cast<size_t>(kMaxOutputBytes)) break;
    }
    return result;
}

static void setChildLimits(int timeoutSec, int memoryLimitMb) {
    struct rlimit rl;

    rl.rlim_cur = static_cast<rlim_t>(timeoutSec);
    rl.rlim_max = static_cast<rlim_t>(timeoutSec + 1);
    setrlimit(RLIMIT_CPU, &rl);

    rlim_t memBytes = static_cast<rlim_t>(memoryLimitMb) * 1024 * 1024;
    rl.rlim_cur = memBytes;
    rl.rlim_max = memBytes + (256UL * 1024 * 1024);
    setrlimit(RLIMIT_AS, &rl);

    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rl);

    rl.rlim_cur = static_cast<rlim_t>(timeoutSec + 2);
    rl.rlim_max = static_cast<rlim_t>(timeoutSec + 2);
    setrlimit(RLIMIT_FSIZE, &rl);
}

std::string ExecutorService::makeTempDir() {
    std::string tmpl = "/tmp/oj_XXXXXX";
    if (mkdtemp(&tmpl[0]) == nullptr) {
        LOG_ERROR("[EXECUTOR] Failed to create temp directory");
        return "";
    }
    return tmpl;
}

bool ExecutorService::writeFile(const std::string &path, const std::string &content) {
    std::ofstream ofs(path);
    if (!ofs) {
        LOG_ERROR("[EXECUTOR] Failed to write file: " + path);
        return false;
    }
    ofs << content;
    ofs.close();
    return ofs.good();
}

bool ExecutorService::compile(const std::string &srcPath,
                               const std::string &binPath,
                               std::string &error) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        error = "Failed to create pipe for compilation";
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        error = "Fork failed";
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        const char *argv[] = {
            "/usr/bin/g++",
            "-std=c++17",
            "-O2",
            "-Wall",
            "-o",
            binPath.c_str(),
            srcPath.c_str(),
            nullptr
        };
        execvp(argv[0], const_cast<char *const *>(argv));
        _exit(1);
    }

    close(pipefd[1]);
    error = readAll(pipefd[0]);
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

TestCaseResult ExecutorService::runTestCase(const std::string &binPath,
                                            const std::string &input,
                                            const std::string &expected,
                                            int position,
                                            int timeoutSec,
                                            int memoryLimitMb) {
    TestCaseResult result;
    result.position = position;
    result.input = input;
    result.expected = expected;

    int stdinPipe[2], stdoutPipe[2];
    if (pipe(stdinPipe) < 0 || pipe(stdoutPipe) < 0) {
        result.status = "RE";
        result.error = "Failed to create pipes";
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        result.status = "RE";
        result.error = "Fork failed";
        close(stdinPipe[0]); close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        return result;
    }

    if (pid == 0) {
        close(stdinPipe[1]);
        close(stdoutPipe[0]);

        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);

        close(stdinPipe[0]);
        close(stdoutPipe[1]);

        setChildLimits(timeoutSec, memoryLimitMb);

        const char *argv[] = { binPath.c_str(), nullptr };
        execvp(argv[0], const_cast<char *const *>(argv));
        _exit(1);
    }

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    ssize_t written = write(stdinPipe[1], input.c_str(), input.size());
    (void)written;
    close(stdinPipe[1]);

    auto start = std::chrono::steady_clock::now();
    int status;
    bool finished = false;

    while (true) {
        int w = waitpid(pid, &status, WNOHANG);
        if (w < 0) {
            result.status = "RE";
            result.error = "waitpid error";
            kill(pid, SIGKILL);
            close(stdoutPipe[0]);
            return result;
        }
        if (w > 0) {
            finished = true;
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start)
                           .count();
        if (elapsed >= static_cast<long long>(timeoutSec) * 1000) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.status = "TLE";
            result.error = "Time Limit Exceeded (" + std::to_string(timeoutSec) + "s)";
            close(stdoutPipe[0]);
            return result;
        }

        usleep(10000);
    }

    result.actual = readAll(stdoutPipe[0]);
    close(stdoutPipe[0]);

    if (!result.actual.empty() && result.actual.back() != '\n') {
        result.actual += '\n';
    }

    if (!finished) {
        result.status = "TLE";
        result.error = "Time Limit Exceeded";
        return result;
    }

    if (WIFEXITED(status)) {
        int exitCode = WEXITSTATUS(status);
        if (exitCode != 0) {
            result.status = "RE";
            result.error = "Runtime Error (exit code " + std::to_string(exitCode) + ")";
            return result;
        }
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        result.status = "RE";
        if (sig == SIGXCPU || sig == SIGALRM) {
            result.status = "TLE";
            result.error = "Time Limit Exceeded";
        } else if (sig == SIGSEGV) {
            result.error = "Segmentation Fault";
        } else {
            result.error = "Killed by signal " + std::to_string(sig);
        }
        return result;
    }

    auto trimTrailing = [](std::string &s) {
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
            s.pop_back();
        }
    };
    std::string normActual = result.actual;
    std::string normExpected = expected;
    trimTrailing(normActual);
    trimTrailing(normExpected);

    if (normActual == normExpected) {
        result.status = "AC";
    } else {
        result.status = "WA";
    }

    return result;
}

ExecutionResult ExecutorService::execute(int problemId, const std::string &code) {
    ExecutionResult execResult;

    Problem problem = ProblemMapper::findById(problemId);
    if (problem.id == 0) {
        execResult.status = "CE";
        execResult.compile_error = "Problem not found";
        return execResult;
    }

    if (problem.test_cases.empty()) {
        execResult.status = "CE";
        execResult.compile_error = "No test cases for this problem";
        return execResult;
    }

    std::string tempDir = makeTempDir();
    if (tempDir.empty()) {
        execResult.status = "CE";
        execResult.compile_error = "Failed to create temp directory";
        return execResult;
    }

    std::string srcPath = tempDir + "/solution.cpp";
    std::string binPath = tempDir + "/solution";

    if (!writeFile(srcPath, code)) {
        execResult.status = "CE";
        execResult.compile_error = "Failed to write source file";
        // Cleanup
        std::remove(srcPath.c_str());
        rmdir(tempDir.c_str());
        return execResult;
    }

    std::string compileError;
    if (!compile(srcPath, binPath, compileError)) {
        execResult.status = "CE";
        execResult.compile_error = compileError;
        std::remove(srcPath.c_str());
        std::remove(binPath.c_str());
        rmdir(tempDir.c_str());
        return execResult;
    }

    execResult.total = static_cast<int>(problem.test_cases.size());

    for (const auto &tc : problem.test_cases) {
        TestCaseResult tcResult = runTestCase(binPath, tc.input, tc.expected,
                                              tc.position,
                                              kDefaultTimeoutSec,
                                              kDefaultMemoryLimitMb);
        execResult.results.push_back(tcResult);
        if (tcResult.status == "AC") {
            execResult.passed++;
        }
    }

    if (execResult.passed == execResult.total) {
        execResult.status = "AC";
    } else {
        for (const auto &r : execResult.results) {
            if (r.status == "CE") {
                execResult.status = "CE";
                break;
            }
            if (r.status == "RE") {
                execResult.status = "RE";
                break;
            }
            if (r.status == "TLE") {
                execResult.status = "TLE";
                break;
            }
            if (r.status == "WA") {
                execResult.status = "WA";
            }
        }
    }

    std::remove(srcPath.c_str());
    std::remove(binPath.c_str());
    rmdir(tempDir.c_str());

    return execResult;
}
