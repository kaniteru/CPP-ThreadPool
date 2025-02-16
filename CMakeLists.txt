cmake_minimum_required(VERSION 3.5)
project(Kani-ThreadPool LANGUAGES CXX)

add_library(Kani-ThreadPool INTERFACE)

target_include_directories(Kani-ThreadPool
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
