cmake_minimum_required(VERSION 3.18)
project(engine)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan #[[REQUIRED]])
if (NOT Vulkan_FOUND)
    set(Vulkan_LIBRARY "")
    set(Vulkan_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/Vulkan-Hpp;${CMAKE_CURRENT_SOURCE_DIR}/../third_party/Vulkan-Hpp/Vulkan-Headers/include")
endif()

add_library(vulkan INTERFACE)
set_target_properties(vulkan PROPERTIES
    INTERFACE_LINK_LIBRARIES "${Vulkan_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS};${CMAKE_CURRENT_SOURCE_DIR}/../third_party/VulkanMemoryAllocator/include"
)

target_compile_definitions(vulkan INTERFACE
    -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    -DVULKAN_HPP_NO_UNION_CONSTRUCTORS
    -DVULKAN_HPP_TYPESAFE_CONVERSION
    -DVK_NO_PROTOTYPES
    -DVMA_STATIC_VULKAN_FUNCTIONS=0
    -DVMA_DYNAMIC_VULKAN_FUNCTIONS=0
)

add_library(engine STATIC
    src/main.cpp
    src/api.hpp
    src/debug.hpp
    src/display.hpp
    src/display.cpp
    src/entity.hpp
    src/vk_mem_alloc.hpp
    src/vk_mem_alloc.cpp
    src/resources/pack.hpp
    src/resources/manager.hpp
    src/resources/resource.hpp
    src/engine.cpp
    src/engine.hpp
    src/scene.hpp
    src/scene.cpp
    src/shared_library.cpp)
target_include_directories(engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(engine PRIVATE glfw)

    target_compile_definitions(engine PUBLIC
        -DGLFW_INCLUDE_NONE
        -DGLFW_INCLUDE_VULKAN
        -DNOMINMAX
    )
    target_compile_definitions(vulkan INTERFACE
        -DVK_USE_PLATFORM_WIN32_KHR
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(vulkan PUBLIC
        -DVK_USE_PLATFORM_XCB_KHR
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
    add_library(android_platform STATIC src/android.cpp)
    target_link_libraries(android_platform PUBLIC android log fmt vulkan)

    target_include_directories(android_platform PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_compile_definitions(vulkan INTERFACE
        -DVK_USE_PLATFORM_ANDROID_KHR
    )

    target_link_libraries(engine PUBLIC android_platform)
endif()

target_link_libraries(engine PUBLIC vulkan fmt)