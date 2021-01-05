cmake_minimum_required(VERSION 3.16)
project(Poll_Server C)

set(CMAKE_C_STANDARD 11)

add_executable(poll-server-telnet poll-server-telnet.c err.c err.h)