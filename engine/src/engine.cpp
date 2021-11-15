#include <engine.hpp>
#include <display.hpp>
#include <debug.hpp>

#include <app.hpp>
#include <iostream>
#include <optional>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include <input/input_system.hpp>

#include <imgui.h>
#include <imgui_layer.hpp>

enum class KeyCode {
    eTab,
    eLeftArrow,
    eRightArrow,
    eUpArrow,
    eDownArrow,
    ePageUp,
    ePageDown,
    eHome,
    eEnd,
    eInsert,
    eDelete,
    eBackspace,
    eSpace,
    eEnter,
    eEscape,
    eKeyPadEnter,
    eA,
    eC,
    eV,
    eX,
    eY,
    eZ,

    eLeftControl,
    eRightControl,
    eLeftShift,
    eRightShift,
    eLeftAlt,
    eRightAlt,
    eLeftSuper,
    eRightSuper
};

namespace vk {
    VULKAN_HPP_STORAGE_API DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

struct JellyEngine::Impl {
    Debug logger{"engine"};
    Display display{"Engine"};
    ImGuiLayer ui{};

    VmaAllocator allocator;

    vk::DebugUtilsMessengerEXT debug_utils;

    vk::DynamicLoader dl;
    vk::Device device;
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice gpu;

    uint32_t graphics_family;
    uint32_t present_family;

    vk::Queue graphics_queue;
    vk::Queue present_queue;

    vk::Extent2D surface_extent;
    vk::PresentModeKHR present_mode;
    vk::SurfaceFormatKHR surface_format;

    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> swapchain_images;
    std::vector<vk::ImageView> swapchain_views;

    std::vector<vk::Fence> fences;
    std::vector<vk::Semaphore> acquire_semaphores;
    std::vector<vk::Semaphore> complete_semaphores;

    std::vector<vk::CommandPool> cmd_pools;

    vk::RenderPass pass;
    std::vector<vk::Framebuffer> framebuffers;

    size_t current_frame = 0;

    /*******************************************************************************************/
//    std::unique_ptr<JellyLayer> layer;

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
};

void JellyEngine::Impl::_initVulkan() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

    _createInstance();
    _createSurface();
    _selectPhysicalDevice();
    _createLogicalDevice();
    _createAllocator();
    _createDebugUtils();
    _createSwapchain();
    _createRenderPass();
    _createFrameBuffers();
    _createCommandPools();
}

void JellyEngine::Impl::_createInstance() {
    const auto layers = std::vector<const char *>{
#if _WIN32
        "VK_LAYER_KHRONOS_validation"
#endif
    };
    const auto app = vk::ApplicationInfo{
        .pApplicationName = nullptr,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Jelly",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2
    };

    auto extensions = display.getInstanceExtensions();
#if _WIN32
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    const auto info = vk::InstanceCreateInfo {
        .pApplicationInfo = &app,
        .enabledLayerCount = static_cast<uint32_t>(std::size(layers)),
        .ppEnabledLayerNames = std::data(layers),
        .enabledExtensionCount = uint32_t(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    instance = vk::createInstance(info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void JellyEngine::Impl::_createSurface() {
    surface = display.createSurface(instance);
}

void JellyEngine::Impl::_selectPhysicalDevice() {
    const auto devices = instance.enumeratePhysicalDevices();

    for (auto device : devices) {
        const auto families = _findQueueFamilies(device);
        if (!families.has_value()) {
            continue;
        }
        if (device.getSurfaceFormatsKHR(surface).empty()) {
            continue;
        }
        if (device.getSurfacePresentModesKHR(surface).empty()) {
            continue;
        }
        gpu = device;
        graphics_family = families->first;
        present_family = families->second;
        break;
    }
}

void JellyEngine::Impl::_createLogicalDevice() {
    const auto layers = std::vector<const char *>{};

    const auto extensions = std::vector<const char *>{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };

    const auto features = vk::PhysicalDeviceFeatures {
        .fillModeNonSolid = true,
        .samplerAnisotropy = true
    };

    const auto priorities = std::array{
        1.0f
    };

    auto infos = std::vector {
        vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = graphics_family,
            .queueCount = 1,
            .pQueuePriorities = priorities.data()
        }
    };

    if (graphics_family != present_family) {
        const auto info = vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = present_family,
            .queueCount = 1,
            .pQueuePriorities = priorities.data()
        };
        infos.emplace_back(info);
    }

    const auto info = vk::DeviceCreateInfo{
        .queueCreateInfoCount = static_cast<uint32_t>(std::size(infos)),
        .pQueueCreateInfos = std::data(infos),
        .enabledLayerCount = static_cast<uint32_t>(std::size(layers)),
        .ppEnabledLayerNames = std::data(layers),
        .enabledExtensionCount = static_cast<uint32_t>(std::size(extensions)),
        .ppEnabledExtensionNames = std::data(extensions),
        .pEnabledFeatures = &features
    };

    device = gpu.createDevice(info, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    present_queue = device.getQueue(present_family, 0);
    graphics_queue = device.getQueue(graphics_family, 0);
}

void JellyEngine::Impl::_createAllocator() {
    const auto functions = VmaVulkanFunctions{
        .vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
        .vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
        .vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
        .vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
        .vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
        .vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
        .vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
        .vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
        .vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
        .vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
        .vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
        .vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
        /// Fetch "vkGetBufferMemoryRequirements2" on Vulkan >= 1.1, fetch "vkGetBufferMemoryRequirements2KHR" when using VK_KHR_dedicated_allocation extension.
        .vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2KHR,
        /// Fetch "vkGetImageMemoryRequirements 2" on Vulkan >= 1.1, fetch "vkGetImageMemoryRequirements2KHR" when using VK_KHR_dedicated_allocation extension.
        .vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2KHR,
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
        /// Fetch "vkBindBufferMemory2" on Vulkan >= 1.1, fetch "vkBindBufferMemory2KHR" when using VK_KHR_bind_memory2 extension.
        .vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2KHR,
        /// Fetch "vkBindImageMemory2" on Vulkan >= 1.1, fetch "vkBindImageMemory2KHR" when using VK_KHR_bind_memory2 extension.
        .vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2KHR,
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
        .vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2KHR,
#endif
    };

    const auto info = VmaAllocatorCreateInfo{
        .flags = {},
        .physicalDevice = gpu,
        .device = device,
        .pVulkanFunctions = &functions,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_2
    };
    vmaCreateAllocator(&info, &allocator);
}

void JellyEngine::Impl::_createDebugUtils() {
#if !_WIN32
    return;
#endif
    const auto info = vk::DebugUtilsMessengerCreateInfoEXT{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = _debugCallback
    };

    debug_utils = instance.createDebugUtilsMessengerEXT(info);
}

void JellyEngine::Impl::_createSwapchain() {
    const auto capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
    const auto formats = gpu.getSurfaceFormatsKHR(surface);
    const auto modes = gpu.getSurfacePresentModesKHR(surface);

    const auto request_formats = std::array {
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eR8G8B8A8Unorm,
        vk::Format::eB8G8R8Unorm,
        vk::Format::eR8G8B8Unorm
    };
    const auto request_modes = std::array{vk::PresentModeKHR::eFifo};
    const auto request_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;

    surface_extent = _selectSurfaceExtent(vk::Extent2D{0, 0}, capabilities);
    surface_format = _selectSurfaceFormat(formats, request_formats, request_color_space);
    present_mode = _selectPresentMode(modes, request_modes);

    auto minImageCount = _getImageCountFromPresentMode(present_mode);
    if (minImageCount < capabilities.minImageCount) {
        minImageCount = capabilities.minImageCount;
    } else if (capabilities.maxImageCount != 0 && minImageCount > capabilities.maxImageCount) {
        minImageCount = capabilities.maxImageCount;
    }

    auto queue_family_indices = std::vector {
        graphics_family
    };
    if (graphics_family != present_family) {
        queue_family_indices.emplace_back(present_family);
    }

    const auto info = vk::SwapchainCreateInfoKHR {
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = surface_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = queue_family_indices.size() == 1
                            ? vk::SharingMode::eExclusive
                            : vk::SharingMode::eConcurrent,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
        .pQueueFamilyIndices = queue_family_indices.data(),
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = true,
        .oldSwapchain = nullptr,
    };

    swapchain = device.createSwapchainKHR(info);
    swapchain_images = device.getSwapchainImagesKHR(swapchain);
    for (const auto image : swapchain_images) {
        const auto view_info = vk::ImageViewCreateInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = surface_format.format,
            .components = {
                .r = vk::ComponentSwizzle::eR,
                .g = vk::ComponentSwizzle::eG,
                .b = vk::ComponentSwizzle::eB,
                .a = vk::ComponentSwizzle::eA,
            },
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        swapchain_views.emplace_back(device.createImageView(view_info));

        const auto fence_info = vk::FenceCreateInfo{
            .flags = vk::FenceCreateFlagBits::eSignaled
        };
        const auto sem_info = vk::SemaphoreCreateInfo{
            .flags = {}
        };

        fences.emplace_back(device.createFence(fence_info));
        acquire_semaphores.emplace_back(device.createSemaphore(sem_info));
        complete_semaphores.emplace_back(device.createSemaphore(sem_info));
    }
}

void JellyEngine::Impl::_createRenderPass() {
    const auto attachments = std::array{
        vk::AttachmentDescription{
            .format = surface_format.format,
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR
        }
    };

    const auto color_attachments = std::array {
        vk::AttachmentReference{
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal
        }
    };

    const auto subpasses = std::array {
        vk::SubpassDescription{
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = static_cast<uint32_t>(color_attachments.size()),
            .pColorAttachments = color_attachments.data(),
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr
        }
    };

    const auto info = vk::RenderPassCreateInfo{
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = subpasses.size(),
        .pSubpasses = subpasses.data(),
        .dependencyCount = 0,
        .pDependencies = nullptr
    };
    pass = device.createRenderPass(info);
}

void JellyEngine::Impl::_createFrameBuffers() {
    for (auto view : swapchain_views) {
        const auto attachments = std::array{
            view
        };
        const auto info = vk::FramebufferCreateInfo{
            .renderPass = pass,
            .attachmentCount = attachments.size(),
            .pAttachments = attachments.data(),
            .width = static_cast<uint32_t>(surface_extent.width),
            .height = static_cast<uint32_t>(surface_extent.height),
            .layers = 1
        };

        framebuffers.emplace_back(device.createFramebuffer(info));
    }
}

void JellyEngine::Impl::_createCommandPools() {
    for (const auto image : swapchain_images) {
        const auto info = vk::CommandPoolCreateInfo{
            .pNext = nullptr,
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphics_family,
        };
        cmd_pools.emplace_back(device.createCommandPool(info));
    }
}

auto JellyEngine::Impl::_findQueueFamilies(vk::PhysicalDevice device) -> std::optional<std::pair<uint32_t, uint32_t>> {
    const auto properties = device.getQueueFamilyProperties();

    uint32_t graphics_family = -1;
    uint32_t present_family = -1;
    for (uint32_t i = 0; i < static_cast<uint32_t>(properties.size()); i++) {
        if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphics_family = i;
        }

        if (device.getSurfaceSupportKHR(i, surface)) {
            present_family = i;
        }

        if ((graphics_family != -1) && (present_family != -1)) {
            return std::pair{graphics_family, present_family};
        }
    }
    return std::nullopt;
}

auto JellyEngine::Impl::_selectSurfaceExtent(
    const vk::Extent2D &extent,
    const vk::SurfaceCapabilitiesKHR &capabilities
) -> vk::Extent2D {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    const auto min = capabilities.minImageExtent;
    const auto max = capabilities.maxImageExtent;

    return {
        std::clamp(extent.width, min.width, max.width),
        std::clamp(extent.height, min.height, max.height)
    };
}

auto JellyEngine::Impl::_selectSurfaceFormat(
    vk::ArrayProxy<const vk::SurfaceFormatKHR> surface_formats,
    vk::ArrayProxy<const vk::Format> request_formats,
    vk::ColorSpaceKHR request_color_space
) -> vk::SurfaceFormatKHR {
    if (surface_formats.size() == 1) {
        if (surface_formats.front().format == vk::Format::eUndefined) {
            return vk::SurfaceFormatKHR {
                .format = request_formats.front(),
                .colorSpace = request_color_space
            };
        }
        return surface_formats.front();
    }

    for (size_t i = 0; i < request_formats.size(); i++) {
        for (size_t k = 0; k < surface_formats.size(); k++) {
            if (surface_formats.data()[k].format != request_formats.data()[i]) {
                continue;
            }
            if (surface_formats.data()[k].colorSpace != request_color_space) {
                continue;
            }
            return surface_formats.data()[k];
        }
    }

    return surface_formats.front();
}

auto JellyEngine::Impl::_selectPresentMode(
    vk::ArrayProxy<const vk::PresentModeKHR> present_modes,
    vk::ArrayProxy<const vk::PresentModeKHR> request_modes
) -> vk::PresentModeKHR {
    for (size_t i = 0; i < request_modes.size(); i++) {
        for (size_t j = 0; j < present_modes.size(); j++) {
            if (request_modes.data()[i] == present_modes.data()[j]) {
                return request_modes.data()[i];
            }
        }
    }
    return vk::PresentModeKHR::eFifo;
}

auto JellyEngine::Impl::_getImageCountFromPresentMode(vk::PresentModeKHR mode) -> uint32_t {
    switch (mode) {
        case vk::PresentModeKHR::eImmediate:
            return 1;
        case vk::PresentModeKHR::eFifo:
        case vk::PresentModeKHR::eFifoRelaxed:
            return 2;
        case vk::PresentModeKHR::eMailbox:
            return 3;
        default:
            return 1;
    }
}

auto JellyEngine::Impl::_debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
) -> VkBool32 {
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        std::cout << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        std::cout << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cout << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << pCallbackData->pMessage << std::endl;
    } else {
        std::cout << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

std::unique_ptr<JellyEngine::Impl> JellyEngine::impl;

JellyEngine::JellyEngine() = default;
JellyEngine::~JellyEngine() = default;

void JellyEngine::initialize() {
    impl = std::make_unique<Impl>();
    impl->_initVulkan();
}

void JellyEngine::run(AppMain& app) {
    app.onAttach();

    while (!impl->display.shouldClose()) {
        impl->display.pollEvents();

        InputSystem::update();

        app.onUpdate();

        const auto color = std::array{1.0f, 0.0f, 0.0f, 1.0f};
        const auto clear_values = std::array{
            vk::ClearValue{.color = {.float32 = color}}
        };
        const auto timeout = std::numeric_limits<uint64_t>::max();
        impl->device.waitForFences(1, &impl->fences[impl->current_frame], true, timeout);
        impl->device.resetFences(1, &impl->fences[impl->current_frame]);

        const auto [_, image_index] = impl->device.acquireNextImageKHR(
            impl->swapchain,
            timeout,
            impl->acquire_semaphores[impl->current_frame]
        );

        const auto begin_info = vk::RenderPassBeginInfo{
            .renderPass = impl->pass,
            .framebuffer = impl->framebuffers[image_index],
            .renderArea = {
                .offset = { .x = 0, .y = 0 },
                .extent = impl->surface_extent
            },
            .clearValueCount = clear_values.size(),
            .pClearValues = clear_values.data()
        };

        const auto cmd_info = vk::CommandBufferAllocateInfo{
            .commandPool = impl->cmd_pools[image_index],
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        auto cmd = impl->device.allocateCommandBuffers(cmd_info).front();

        cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        cmd.beginRenderPass(begin_info, vk::SubpassContents::eInline);

        app.onRender();

        cmd.endRenderPass();
        cmd.end();

        const auto wait_semaphores = std::array{
            impl->acquire_semaphores[impl->current_frame]
        };

        const auto signal_semaphores = std::array{
            impl->complete_semaphores[impl->current_frame]
        };

        const auto stages = std::array{
            vk::PipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput}
        };

        const auto submit_info = vk::SubmitInfo {
            .waitSemaphoreCount = wait_semaphores.size(),
            .pWaitSemaphores = wait_semaphores.data(),
            .pWaitDstStageMask = stages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = signal_semaphores.size(),
            .pSignalSemaphores = signal_semaphores.data()
        };

        impl->graphics_queue.submit(std::array{submit_info}, impl->fences[impl->current_frame]);

        const auto present_info = vk::PresentInfoKHR{
            .waitSemaphoreCount = signal_semaphores.size(),
            .pWaitSemaphores = signal_semaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &impl->swapchain,
            .pImageIndices = &image_index
        };
        impl->present_queue.presentKHR(present_info);
        impl->present_queue.waitIdle();

        impl->device.freeCommandBuffers(impl->cmd_pools[image_index], std::array{cmd});

        impl->current_frame = (impl->current_frame + 1) % impl->swapchain_images.size();
    }

    app.onDetach();
}