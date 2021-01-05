cmake_minimum_required(VERSION 3.16)
project(zad10 C)

set(CMAKE_C_STANDARD 99)

add_executable(zad10 zad10/file-le-client.c zad10/file-le-server.c zad10/err.c zad10/err.h)