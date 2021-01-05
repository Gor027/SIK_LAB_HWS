cmake_minimum_required(VERSION 3.16)
project(Zad8 C)

set(CMAKE_C_STANDARD 11)

add_executable(exec err.c err.h multi-recv.c multi-recv6.c multi-send.c multi-send6.c ../zad8/time-server.c ../zad8/time-client.c)