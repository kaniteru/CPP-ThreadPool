cmake_minimum_required(VERSION 3.5)
project(ThreadPool LANGUAGES CXX)

add_library(ThreadPool INTERFACE)

target_include_directories(ThreadPool
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
