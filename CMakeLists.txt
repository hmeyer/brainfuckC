cmake_minimum_required(VERSION 3.6)
project(bfs)
SET (CMAKE_C_COMPILER             "/usr/bin/clang")
SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(SOURCE bfs.cc
    scanner.hpp scanner.cc
    token.hpp token.cc
    expression.hpp expression.cc
    statement.hpp statement.cc
    parser.hpp parser.cc
    bf_space.hpp bf_space.cc)
add_executable(${PROJECT_NAME} ${SOURCE})

add_executable(bfi bfi.cc)