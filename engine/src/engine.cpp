#include <engine.hpp>
#include <display.hpp>
#include <debug.hpp>

#include <iostream>
#include <optional>
#include <fmt/format.h>

#include <vk_mem_alloc.hpp>
#include <shared_library.hpp>

#include <scene.hpp>
#include <entity.hpp>

namespace vk {
    VULKAN_HPP_STORAGE_API DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

struct Engine::Impl {
    Debug logger{"engine"};
    Display display{"Engine"};

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

    std::shared_ptr<Scene> scene = nullptr;
};

Engine::Engine()  {
    impl = std::make_unique<Impl>();
    _initVulkan();
}

Engine::~Engine() {}

auto Engine::loadScene(const std::string &name) -> bool {
    auto lib = SharedLibrary::open(name);
    if (!lib) {
        impl->logger.error(fmt::format("Scene '{}' not found", name));
        return false;
    }

    auto load = lib->get<void(*)(Scene&) > ("LoadScene");
    if (load == nullptr) {
        impl->logger.error(fmt::format("Scene '{}' not loaded", name));
        return false;
    }
    impl->scene.reset(new Scene(std::move(*lib)));

    load(*impl->scene);

    return true;
}

void Engine::run() {
//#if _WIN32
//        if (!loadScene("game.dll")) {
//            return;
//        }
//#else
//        if (!loadScene("libgame.so")) {
//            return;
//        }
//#endif

    while (!impl->display.shouldClose()) {
        impl->display.pollEvents();

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
        /*
         *
         *
         *
         *
         *
         *
         *
         *
         *
         * */
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
}

void Engine::_initVulkan() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(impl->dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

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

void Engine::_createInstance() {
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

    auto extensions = impl->display.getInstanceExtensions();
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

    impl->instance = vk::createInstance(info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(impl->instance);
}

void Engine::_createSurface() {
    impl->surface = impl->display.createSurface(impl->instance);
}

void Engine::_selectPhysicalDevice() {
    const auto devices = impl->instance.enumeratePhysicalDevices();

    for (auto device : devices) {
        const auto families = _findQueueFamilies(device);
        if (!families.has_value()) {
            continue;
        }
        if (device.getSurfaceFormatsKHR(impl->surface).empty()) {
            continue;
        }
        if (device.getSurfacePresentModesKHR(impl->surface).empty()) {
            continue;
        }
        impl->gpu = device;
        impl->graphics_family = families->first;
        impl->present_family = families->second;
        break;
    }
}

void Engine::_createLogicalDevice() {
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
            .queueFamilyIndex = impl->graphics_family,
            .queueCount = 1,
            .pQueuePriorities = priorities.data()
        }
    };

    if (impl->graphics_family != impl->present_family) {
        const auto info = vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = impl->present_family,
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

    impl->device = impl->gpu.createDevice(info, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(impl->device);

    impl->present_queue = impl->device.getQueue(impl->present_family, 0);
    impl->graphics_queue = impl->device.getQueue(impl->graphics_family, 0);
}

void Engine::_createAllocator() {
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
        .physicalDevice = impl->gpu,
        .device = impl->device,
        .pVulkanFunctions = &functions,
        .instance = impl->instance,
        .vulkanApiVersion = VK_API_VERSION_1_2
    };
    vmaCreateAllocator(&info, &impl->allocator);
}

void Engine::_createDebugUtils() {
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

    impl->debug_utils = impl->instance.createDebugUtilsMessengerEXT(info);
}

void Engine::_createSwapchain() {
    const auto capabilities = impl->gpu.getSurfaceCapabilitiesKHR(impl->surface);
    const auto formats = impl->gpu.getSurfaceFormatsKHR(impl->surface);
    const auto modes = impl->gpu.getSurfacePresentModesKHR(impl->surface);

    const auto request_formats = std::array {
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eR8G8B8A8Unorm,
        vk::Format::eB8G8R8Unorm,
        vk::Format::eR8G8B8Unorm
    };
    const auto request_modes = std::array{vk::PresentModeKHR::eFifo};
    const auto request_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;

    impl->surface_extent = _selectSurfaceExtent(vk::Extent2D{0, 0}, capabilities);
    impl->surface_format = _selectSurfaceFormat(formats, request_formats, request_color_space);
    impl->present_mode = _selectPresentMode(modes, request_modes);

    auto minImageCount = _getImageCountFromPresentMode(impl->present_mode);
    if (minImageCount < capabilities.minImageCount) {
        minImageCount = capabilities.minImageCount;
    } else if (capabilities.maxImageCount != 0 && minImageCount > capabilities.maxImageCount) {
        minImageCount = capabilities.maxImageCount;
    }

    auto queue_family_indices = std::vector {
            impl->graphics_family
    };
    if (impl->graphics_family != impl->present_family) {
        queue_family_indices.emplace_back(impl->present_family);
    }

    const auto info = vk::SwapchainCreateInfoKHR {
        .surface = impl->surface,
        .minImageCount = minImageCount,
        .imageFormat = impl->surface_format.format,
        .imageColorSpace = impl->surface_format.colorSpace,
        .imageExtent = impl->surface_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = queue_family_indices.size() == 1
                            ? vk::SharingMode::eExclusive
                            : vk::SharingMode::eConcurrent,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
        .pQueueFamilyIndices = queue_family_indices.data(),
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = impl->present_mode,
        .clipped = true,
        .oldSwapchain = nullptr,
    };

    impl->swapchain = impl->device.createSwapchainKHR(info);
    impl->swapchain_images = impl->device.getSwapchainImagesKHR(impl->swapchain);
    for (const auto image : impl->swapchain_images) {
        const auto view_info = vk::ImageViewCreateInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = impl->surface_format.format,
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
        impl->swapchain_views.emplace_back(impl->device.createImageView(view_info));

        const auto fence_info = vk::FenceCreateInfo{
            .flags = vk::FenceCreateFlagBits::eSignaled
        };
        const auto sem_info = vk::SemaphoreCreateInfo{
            .flags = {}
        };

        impl->fences.emplace_back(impl->device.createFence(fence_info));
        impl->acquire_semaphores.emplace_back(impl->device.createSemaphore(sem_info));
        impl->complete_semaphores.emplace_back(impl->device.createSemaphore(sem_info));
    }
}

void Engine::_createRenderPass() {
    const auto attachments = std::array{
        vk::AttachmentDescription{
            .format = impl->surface_format.format,
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
    impl->pass = impl->device.createRenderPass(info);
}

void Engine::_createFrameBuffers() {
    for (auto view : impl->swapchain_views) {
        const auto attachments = std::array{
            view
        };
        const auto info = vk::FramebufferCreateInfo{
            .renderPass = impl->pass,
            .attachmentCount = attachments.size(),
            .pAttachments = attachments.data(),
            .width = static_cast<uint32_t>(impl->surface_extent.width),
            .height = static_cast<uint32_t>(impl->surface_extent.height),
            .layers = 1
        };

        impl->framebuffers.emplace_back(impl->device.createFramebuffer(info));
    }
}

void Engine::_createCommandPools() {
    for (const auto image : impl->swapchain_images) {
        const auto info = vk::CommandPoolCreateInfo{
            .pNext = nullptr,
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = impl->graphics_family,
        };
        impl->cmd_pools.emplace_back(impl->device.createCommandPool(info));
    }
}

auto Engine::_findQueueFamilies(vk::PhysicalDevice device) -> std::optional<std::pair<uint32_t, uint32_t>> {
    const auto properties = device.getQueueFamilyProperties();

    uint32_t graphics_family = -1;
    uint32_t present_family = -1;
    for (uint32_t i = 0; i < static_cast<uint32_t>(properties.size()); i++) {
        if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphics_family = i;
        }

        if (device.getSurfaceSupportKHR(i, impl->surface)) {
            present_family = i;
        }

        if ((graphics_family != -1) && (present_family != -1)) {
            return std::pair{graphics_family, present_family};
        }
    }
    return std::nullopt;
}

auto Engine::_selectSurfaceExtent(
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

auto Engine::_selectSurfaceFormat(
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

auto Engine::_selectPresentMode(
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

auto Engine::_getImageCountFromPresentMode(vk::PresentModeKHR mode) -> uint32_t {
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

auto Engine::_debugCallback(
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