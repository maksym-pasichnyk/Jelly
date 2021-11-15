#pragma once

#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>

struct Display {
    struct Impl;

    explicit Display(const std::string& title);
    ~Display();

    void pollEvents();
    auto shouldClose() -> bool;
    auto createSurface(vk::Instance instance) -> vk::SurfaceKHR;
    auto getInstanceExtensions() -> std::vector<const char *>;

private:
    std::unique_ptr<Impl> impl;
};