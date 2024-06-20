cmake_minimum_required(VERSION 3.0)

project(servers)

add_executable(multi-accept multi-accept.cpp)
add_executable(multi-accept-std-thread multi-accept-std-thread.cpp)
add_executable(queue queue.cpp)
