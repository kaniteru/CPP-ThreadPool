cmake_minimum_required(VERSION 3.5)
project(ThreadPool LANGUAGES CXX)

add_library(ThreadPool INTERFACE)

target_include_directories(ThreadPool
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

#test
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(CMAKE_CXX_STANDARD 11)
    add_executable(Test test/test.cpp)
    target_link_libraries(Test PRIVATE ThreadPool)
endif()