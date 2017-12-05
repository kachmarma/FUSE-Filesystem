cmake_minimum_required(VERSION 3.8)
project(CH03)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES ch03-assignment/bmap.c ch03-assignment/bmap.h ch03-assignment/nufs.c ch03-assignment/storage.c
        ch03-assignment/storage.h)
add_executable(BattleZedClient ${SOURCE_FILES})