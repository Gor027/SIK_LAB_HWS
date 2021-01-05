cmake_minimum_required(VERSION 3.15)
project(Zad6 C)
set(CMAKE_C_STANDARD 11)
add_executable(Zad6 file-client-tcp.c file-server-tcp.c err.h err.c)