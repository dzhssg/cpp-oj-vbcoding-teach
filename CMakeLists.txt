cmake_minimum_required(VERSION 3.16)
project(cpp_oj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── System dependencies ──────────────────────────────────────────────

find_package(yaml-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

# ── FetchContent: cpp-httplib / nlohmann::json ─────────────

include(FetchContent)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.3
)
FetchContent_MakeAvailable(cpp-httplib)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# ── Source files ─────────────────────────────────────────────────────

set(SOURCES
    src/main.cc
    src/server/server.cc
    src/server/router.cc
    src/handler/problem_handler.cc
    src/handler/submit_handler.cc
    src/handler/auth_handler.cc
    src/handler/admin_handler.cc
    src/service/problem_service.cc
    src/service/executor_service.cc
    src/service/auth_service.cc
    src/model/problem.cc
    src/model/test_case.cc
    src/model/user.cc
    src/db/connection_pool.cc
    src/utils/logger.cc
    src/utils/config.cc
)

# ── Executable ───────────────────────────────────────────────────────

link_directories(${MYSQL_LIBRARY_DIRS})

add_executable(oj_server ${SOURCES})

target_include_directories(oj_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${MYSQL_INCLUDE_DIRS}
)

target_link_libraries(oj_server PRIVATE
    yaml-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    httplib::httplib
    nlohmann_json::nlohmann_json
    ${MYSQL_LIBRARIES}
    pthread
)

# ── Tests (optional) ─────────────────────────────────────────────────

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
