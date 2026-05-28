cmake_minimum_required(VERSION 3.16)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

set(UNIT_TESTS
    unit/logger_test.cc
    unit/config_test.cc
)

foreach(test_src ${UNIT_TESTS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src}
        ${CMAKE_SOURCE_DIR}/src/utils/logger.cc
        ${CMAKE_SOURCE_DIR}/src/utils/config.cc
    )
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${test_name} PRIVATE
        gtest_main
        gtest
        yaml-cpp
        pthread
        stdc++fs
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
