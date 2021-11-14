cmake_minimum_required(VERSION 3.18)
project(engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DWIN32_LEAN_AND_MEAN)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_subdirectory(third_party/glfw)
endif()

add_subdirectory(third_party/fmt)
add_subdirectory(engine)
add_subdirectory(example)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_executable(engine-main src/main.cpp)
else()
    add_library(engine-main SHARED src/main.cpp)
endif()
target_link_libraries(engine-main PRIVATE engine)
target_include_directories(engine-main PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_dependencies(engine-main example)