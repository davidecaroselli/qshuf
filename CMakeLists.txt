cmake_minimum_required(VERSION 3.30)
project(qshuf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(qshuf qshuf.cpp)
