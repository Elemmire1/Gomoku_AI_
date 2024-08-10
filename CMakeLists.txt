cmake_minimum_required(VERSION 3.16)
project(Gomoku_AI_main)

set(CMAKE_CXX_STANDARD 14)

include_directories(judge)

add_executable(Gomoku_AI_main
        judge/AIController.h
        judge/sample.cpp
)
