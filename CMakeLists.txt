cmake_minimum_required(VERSION 3.16)
project(miniredis CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Main server
add_executable(miniredis
    src/main.cpp
    src/server.cpp
    src/store.cpp
    src/resp.cpp
    src/command_handler.cpp
)
target_include_directories(miniredis PRIVATE src)
target_link_libraries(miniredis pthread)

# Tests
enable_testing()
find_package(GTest QUIET)
if(GTest_FOUND)
    add_executable(miniredis_tests
        tests/test_store.cpp
        tests/test_resp.cpp
        src/store.cpp
        src/resp.cpp
        src/command_handler.cpp
    )
    target_include_directories(miniredis_tests PRIVATE src)
    target_link_libraries(miniredis_tests GTest::gtest_main pthread)
    add_test(NAME miniredis_tests COMMAND miniredis_tests)
endif()
