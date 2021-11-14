#pragma once

#include <string>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

struct Engine {
    Engine();
    ~Engine();

    auto loadScene(const std::string& name) -> bool;
    void run();

private:
    struct Impl;

    void _initVulkan();
    void _createInstance();
    void _createSurface();
    void _selectPhysicalDevice();
    void _createLogicalDevice();
    void _createAllocator();
    void _createDebugUtils();
    void _createSwapchain();
    void _createRenderPass();
    void _createFrameBuffers();
    void _createCommandPools();
    auto _findQueueFamilies(vk::PhysicalDevice device) -> std::optional<std::pair<uint32_t, uint32_t>>;
    auto _selectSurfaceExtent(
        const vk::Extent2D &extent,
        const vk::SurfaceCapabilitiesKHR &capabilities
    ) -> vk::Extent2D;
    auto _selectSurfaceFormat(
            vk::ArrayProxy<const vk::SurfaceFormatKHR> surface_formats,
        vk::ArrayProxy<const vk::Format> request_formats,
        vk::ColorSpaceKHR request_color_space
    ) -> vk::SurfaceFormatKHR;
    auto _selectPresentMode(
        vk::ArrayProxy<const vk::PresentModeKHR> present_modes,
        vk::ArrayProxy<const vk::PresentModeKHR> request_modes
    ) -> vk::PresentModeKHR;
    auto _getImageCountFromPresentMode(vk::PresentModeKHR mode) -> uint32_t;

    static VKAPI_ATTR auto VKAPI_CALL _debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData
    ) -> VkBool32;

private:
    std::unique_ptr<Impl> impl;
};
