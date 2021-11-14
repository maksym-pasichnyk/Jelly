cmake_minimum_required(VERSION 3.18)
project(example)

set(CMAKE_CXX_STANDARD 20)

add_library(example SHARED game.cpp)
target_link_libraries(example PRIVATE engine)