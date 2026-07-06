FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

link_directories(${MYSQL_LIBRARY_DIRS})

function(add_unit_test test_name)
    add_executable(${test_name} ${ARGN})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${MYSQL_INCLUDE_DIRS}
    )
    target_link_libraries(${test_name} PRIVATE
        gtest_main
        gtest
        yaml-cpp
        pthread
        stdc++fs
        ${MYSQL_LIBRARIES}
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_unit_test(logger_test
    unit/logger_test.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(config_test
    unit/config_test.cc
    ${CMAKE_SOURCE_DIR}/src/utils/config.cc
)

add_unit_test(connection_pool_test
    unit/connection_pool_test.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(problem_mapper_test
    unit/problem_mapper_test.cc
    ${CMAKE_SOURCE_DIR}/src/model/problem.cc
    ${CMAKE_SOURCE_DIR}/src/model/test_case.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(test_case_mapper_test
    unit/test_case_mapper_test.cc
    ${CMAKE_SOURCE_DIR}/src/model/test_case.cc
    ${CMAKE_SOURCE_DIR}/src/model/problem.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(user_mapper_test
    unit/user_mapper_test.cc
    ${CMAKE_SOURCE_DIR}/src/model/user.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(problem_service_test
    unit/problem_test.cc
    ${CMAKE_SOURCE_DIR}/src/service/problem_service.cc
    ${CMAKE_SOURCE_DIR}/src/model/problem.cc
    ${CMAKE_SOURCE_DIR}/src/model/test_case.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(executor_service_test
    unit/executor_test.cc
    ${CMAKE_SOURCE_DIR}/src/service/executor_service.cc
    ${CMAKE_SOURCE_DIR}/src/model/problem.cc
    ${CMAKE_SOURCE_DIR}/src/model/test_case.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(session_mapper_test
    unit/session_mapper_test.cc
    ${CMAKE_SOURCE_DIR}/src/model/session.cc
    ${CMAKE_SOURCE_DIR}/src/model/user.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)

add_unit_test(session_service_test
    unit/session_service_test.cc
    ${CMAKE_SOURCE_DIR}/src/service/session_service.cc
    ${CMAKE_SOURCE_DIR}/src/model/session.cc
    ${CMAKE_SOURCE_DIR}/src/model/user.cc
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)
target_link_libraries(session_service_test PRIVATE OpenSSL::SSL OpenSSL::Crypto)

add_unit_test(auth_handler_test
    unit/auth_handler_test.cc
    ${CMAKE_SOURCE_DIR}/src/handler/auth_handler.cc
    ${CMAKE_SOURCE_DIR}/src/service/auth_service.cc
    ${CMAKE_SOURCE_DIR}/src/service/session_service.cc
    ${CMAKE_SOURCE_DIR}/src/model/session.cc
    ${CMAKE_SOURCE_DIR}/src/model/user.cc
    ${CMAKE_SOURCE_DIR}/src/utils/bcrypt.cpp
    ${CMAKE_SOURCE_DIR}/src/db/connection_pool.cc
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
)
target_link_libraries(auth_handler_test PRIVATE OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

add_unit_test(bcrypt_test
    unit/bcrypt_test.cc
    ${CMAKE_SOURCE_DIR}/src/utils/bcrypt.cpp
)
target_link_libraries(bcrypt_test PRIVATE OpenSSL::SSL OpenSSL::Crypto)
