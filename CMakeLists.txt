set(CMAKE_C_COMPILER "clang-15")
set(CMAKE_CXX_COMPILER "clang++-15")

cmake_minimum_required(VERSION 3.0.0)
project(network VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

include(CTest)
enable_testing()

aux_source_directory(. DIR_SRCS)

add_executable(network main.cpp)
add_subdirectory(event_loop)
add_subdirectory(logger)
add_subdirectory(time_utils)
add_subdirectory(tcp)
add_subdirectory(buffer)
add_subdirectory(test)
add_subdirectory(poller)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(
    network
    stdc++
    pthread
    fmt
    gtest
    gtest_main

    Test_Lib
    TCP_Lib
    Event_Loop_lib
    Logger
    Buffer_Lib
    Time_Utils_Lib
    Poller_Lib
)