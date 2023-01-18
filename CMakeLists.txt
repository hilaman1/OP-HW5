cmake_minimum_required(VERSION 3.0)
project(HW5 C)

set(CMAKE_C_STANDARD 11)

add_executable(HW5 pcc_server.c)
target_link_libraries(HW5 m)