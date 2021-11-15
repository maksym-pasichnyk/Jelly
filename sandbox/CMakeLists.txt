cmake_minimum_required(VERSION 3.18)
project(sandbox)

set(CMAKE_CXX_STANDARD 20)

add_executable(sandbox src/main.cpp)
target_link_libraries(sandbox PRIVATE engine)