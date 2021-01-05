cmake_minimum_required(VERSION 3.16)
project(zad9 C)

set(CMAKE_C_STANDARD 99)

add_executable(zad9 err.h err.c dropnobody.c in_cksum.c myping.c)