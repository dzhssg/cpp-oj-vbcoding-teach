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
