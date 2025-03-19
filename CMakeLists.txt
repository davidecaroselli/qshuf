cmake_minimum_required(VERSION 3.22)
project(qshuf)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif ()
message(STATUS "Building project in ${CMAKE_BUILD_TYPE} mode")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(qshuf qshuf.cpp)
