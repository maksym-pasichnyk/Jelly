#include <display.hpp>
#include <debug.hpp>

#include <list>
#include <vector>
#include <iostream>
#include <optional>
#include <fmt/format.h>

#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include <shared_library.hpp>

#include <scene.hpp>
#include <entity.hpp>

namespace vk {
    VULKAN_HPP_STORAGE_API DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

struct AppTest {
    std::shared_ptr<Scene> scene = nullptr;

    AppTest() : self {
        .display = {"AppTest"}
    } {
        _initVulkan();
    }
    ~AppTest() {}

    auto loadScene(const std::string& name) -> bool {
        auto lib = SharedLibrary::open(name);
        if (!lib) {
            self.logger.error(fmt::format("Scene '{}' not found", name));
            return false;
        }

        auto load = lib->get<void(*)(Scene&) > ("LoadScene");
        if (load == nullptr) {
            self.logger.error(fmt::format("Scene '{}' not loaded", name));
            return false;
        }
        scene.reset(new Scene(std::move(*lib)));

        load(*scene);

        return true;
    }

    void run() {
//#if _WIN32
//        if (!loadScene("game.dll")) {
//            return;
//        }
//#else
//        if (!loadScene("libgame.so")) {
//            return;
//        }
//#endif

        while (!self.display.shouldClose()) {
            self.display.pollEvents();

            const auto color = std::array{1.0f, 0.0f, 0.0f, 1.0f};
            const auto clear_values = std::array{
                vk::ClearValue{.color = {.float32 = color}}
            };
            const auto timeout = std::numeric_limits<uint64_t>::max();
            self.device.waitForFences(1, &self.fences[self.current_frame], true, timeout);
            self.device.resetFences(1, &self.fences[self.current_frame]);

            const auto [_, image_index] = self.device.acquireNextImageKHR(
                self.swapchain,
                timeout,
                self.acquire_semaphores[self.current_frame]
            );

            const auto begin_info = vk::RenderPassBeginInfo{
                .renderPass = self.pass,
                .framebuffer = self.framebuffers[image_index],
                .renderArea = {
                    .offset = { .x = 0, .y = 0 },
                    .extent = self.surface_extent
                },
                .clearValueCount = clear_values.size(),
                .pClearValues = clear_values.data()
            };

            const auto cmd_info = vk::CommandBufferAllocateInfo{
                .commandPool = self.cmd_pools[image_index],
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1
            };
            auto cmd = self.device.allocateCommandBuffers(cmd_info).front();

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
                self.acquire_semaphores[self.current_frame]
            };

            const auto signal_semaphores = std::array{
                self.complete_semaphores[self.current_frame]
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

            self.graphics_queue.submit(std::array{submit_info}, self.fences[self.current_frame]);

            const auto present_info = vk::PresentInfoKHR{
                .waitSemaphoreCount = signal_semaphores.size(),
                .pWaitSemaphores = signal_semaphores.data(),
                .swapchainCount = 1,
                .pSwapchains = &self.swapchain,
                .pImageIndices = &image_index
            };
            self.present_queue.presentKHR(present_info);
            self.present_queue.waitIdle();

            self.device.freeCommandBuffers(self.cmd_pools[image_index], std::array{cmd});

            self.current_frame = (self.current_frame + 1) % self.swapchain_images.size();
        }
    }

private:
    void _initVulkan() {
        VULKAN_HPP_DEFAULT_DISPATCHER.init(self.dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

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

    void _createInstance() {
        const auto layers = std::vector<const char *>{
#if _WIN32
            "VK_LAYER_KHRONOS_validation"
#endif
        };
        const auto app = vk::ApplicationInfo{
            .pApplicationName = nullptr,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Craft Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_2
        };

        auto extensions = self.display.getInstanceExtensions();
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

        self.instance = vk::createInstance(info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(self.instance);
    }

    void _createSurface() {
        self.surface = self.display.createSurface(self.instance);
    }

    void _selectPhysicalDevice() {
        const auto devices = self.instance.enumeratePhysicalDevices();

        for (auto device : devices) {
            const auto families = _findQueueFamilies(device);
            if (!families.has_value()) {
                continue;
            }
            if (device.getSurfaceFormatsKHR(self.surface).empty()) {
                continue;
            }
            if (device.getSurfacePresentModesKHR(self.surface).empty()) {
                continue;
            }
            self.gpu = device;
            self.graphics_family = families->first;
            self.present_family = families->second;
            break;
        }
    }

    void _createLogicalDevice() {
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
                .queueFamilyIndex = self.graphics_family,
                .queueCount = 1,
                .pQueuePriorities = priorities.data()
            }
        };

        if (self.graphics_family != self.present_family) {
            const auto info = vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = self.present_family,
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

        self.device = self.gpu.createDevice(info, nullptr);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(self.device);

        self.present_queue = self.device.getQueue(self.present_family, 0);
        self.graphics_queue = self.device.getQueue(self.graphics_family, 0);
    }

    void _createAllocator() {
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
            .physicalDevice = self.gpu,
            .device = self.device,
            .pVulkanFunctions = &functions,
            .instance = self.instance,
            .vulkanApiVersion = VK_API_VERSION_1_2
        };
        vmaCreateAllocator(&info, &self.allocator);
    }

    void _createDebugUtils() {
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

        self.debug_utils = self.instance.createDebugUtilsMessengerEXT(info);
    }

    void _createSwapchain() {
        const auto capabilities = self.gpu.getSurfaceCapabilitiesKHR(self.surface);
        const auto formats = self.gpu.getSurfaceFormatsKHR(self.surface);
        const auto modes = self.gpu.getSurfacePresentModesKHR(self.surface);

        const auto request_formats = std::array {
            vk::Format::eB8G8R8A8Unorm,
            vk::Format::eR8G8B8A8Unorm,
            vk::Format::eB8G8R8Unorm,
            vk::Format::eR8G8B8Unorm
        };
        const auto request_modes = std::array{vk::PresentModeKHR::eFifo};
        const auto request_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;

        self.surface_extent = _selectSurfaceExtent(vk::Extent2D{0, 0}, capabilities);
        self.surface_format = _selectSurfaceFormat(formats, request_formats, request_color_space);
        self.present_mode = _selectPresentMode(modes, request_modes);

        auto minImageCount = _getImageCountFromPresentMode(self.present_mode);
        if (minImageCount < capabilities.minImageCount) {
            minImageCount = capabilities.minImageCount;
        } else if (capabilities.maxImageCount != 0 && minImageCount > capabilities.maxImageCount) {
            minImageCount = capabilities.maxImageCount;
        }

        auto queue_family_indices = std::vector {
            self.graphics_family
        };
        if (self.graphics_family != self.present_family) {
            queue_family_indices.emplace_back(self.present_family);
        }

        const auto info = vk::SwapchainCreateInfoKHR {
            .surface = self.surface,
            .minImageCount = minImageCount,
            .imageFormat = self.surface_format.format,
            .imageColorSpace = self.surface_format.colorSpace,
            .imageExtent = self.surface_extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = queue_family_indices.size() == 1
                                ? vk::SharingMode::eExclusive
                                : vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
            .pQueueFamilyIndices = queue_family_indices.data(),
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = self.present_mode,
            .clipped = true,
            .oldSwapchain = nullptr,
        };

        self.swapchain = self.device.createSwapchainKHR(info);
        self.swapchain_images = self.device.getSwapchainImagesKHR(self.swapchain);
        for (const auto image : self.swapchain_images) {
            const auto view_info = vk::ImageViewCreateInfo{
                .image = image,
                .viewType = vk::ImageViewType::e2D,
                .format = self.surface_format.format,
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
            self.swapchain_views.emplace_back(self.device.createImageView(view_info));

            const auto fence_info = vk::FenceCreateInfo{
                .flags = vk::FenceCreateFlagBits::eSignaled
            };
            const auto sem_info = vk::SemaphoreCreateInfo{
                .flags = {}
            };

            self.fences.emplace_back(self.device.createFence(fence_info));
            self.acquire_semaphores.emplace_back(self.device.createSemaphore(sem_info));
            self.complete_semaphores.emplace_back(self.device.createSemaphore(sem_info));
        }
    }

    void _createRenderPass() {
        const auto attachments = std::array{
            vk::AttachmentDescription{
                .format = self.surface_format.format,
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
        self.pass = self.device.createRenderPass(info);
    }

    void _createFrameBuffers() {
        for (auto view : self.swapchain_views) {
            const auto attachments = std::array{
                view
            };
            const auto info = vk::FramebufferCreateInfo{
                .renderPass = self.pass,
                .attachmentCount = attachments.size(),
                .pAttachments = attachments.data(),
                .width = static_cast<uint32_t>(self.surface_extent.width),
                .height = static_cast<uint32_t>(self.surface_extent.height),
                .layers = 1
            };

            self.framebuffers.emplace_back(self.device.createFramebuffer(info));
        }
    }

    void _createCommandPools() {
        for (const auto image : self.swapchain_images) {
            const auto info = vk::CommandPoolCreateInfo{
                .pNext = nullptr,
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = self.graphics_family,
            };
            self.cmd_pools.emplace_back(self.device.createCommandPool(info));
        }
    }

    auto _findQueueFamilies(vk::PhysicalDevice device) -> std::optional<std::pair<uint32_t, uint32_t>> {
        const auto properties = device.getQueueFamilyProperties();

        uint32_t graphics_family = -1;
        uint32_t present_family = -1;
        for (uint32_t i = 0; i < static_cast<uint32_t>(properties.size()); i++) {
            if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                graphics_family = i;
            }

            if (device.getSurfaceSupportKHR(i, self.surface)) {
                present_family = i;
            }

            if ((graphics_family != -1) && (present_family != -1)) {
                return std::pair{graphics_family, present_family};
            }
        }
        return std::nullopt;
    }

    auto _selectSurfaceExtent(
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

    auto _selectSurfaceFormat(
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

    auto _selectPresentMode(
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

    auto _getImageCountFromPresentMode(vk::PresentModeKHR mode) -> uint32_t {
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

    static VKAPI_ATTR auto VKAPI_CALL _debugCallback(
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

    struct {
        Debug logger{"engine"};

        VmaAllocator allocator;
        Display display;

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
    } self;
};

int main() {
    AppTest app{};
    app.run();
    return 0;
}
