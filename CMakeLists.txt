cmake_minimum_required(VERSION 3.18)
project(engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DWIN32_LEAN_AND_MEAN)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_subdirectory(third_party/glfw)
endif()

set(IMGUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
set(VULKAN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Hpp")

add_subdirectory(third_party/fmt)
add_subdirectory(imgui)
add_subdirectory(engine)
add_subdirectory(sandbox)

if (CMAKE_SYSTEM_NAME MATCHES "Android")
    add_library(engine-main SHARED src/android-main.cpp)
    target_link_libraries(engine-main PRIVATE engine)
    target_include_directories(engine-main PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
endif()
