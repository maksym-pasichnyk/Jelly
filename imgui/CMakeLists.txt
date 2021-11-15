cmake_minimum_required(VERSION 3.18)
project(engine)

set(CMAKE_CXX_STANDARD 20)

add_library(imgui-static STATIC
    "${IMGUI_SOURCE_DIR}/imgui.h"
    "${IMGUI_SOURCE_DIR}/imconfig.h"
    "${IMGUI_SOURCE_DIR}/imgui_internal.h"
    "${IMGUI_SOURCE_DIR}/imgui.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_draw.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_tables.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_layer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_layer.cpp"
)
target_include_directories(imgui-static PRIVATE "${IMGUI_SOURCE_DIR}")

add_library(imgui INTERFACE
    "${IMGUI_SOURCE_DIR}/imgui.h"
    "${IMGUI_SOURCE_DIR}/imconfig.h"
    "${IMGUI_SOURCE_DIR}/imgui_internal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_layer.hpp")

set_target_properties(imgui PROPERTIES
    INTERFACE_LINK_LIBRARIES imgui-static
    INTERFACE_INCLUDE_DIRECTORIES "${IMGUI_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}"
)