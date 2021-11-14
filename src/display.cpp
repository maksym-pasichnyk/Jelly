#include "display.hpp"

#if _WIN32
#include <GLFW/glfw3.h>
#endif

#if _WIN32
struct Display::Impl {
    GLFWwindow* window;

    Impl(const std::string& title) {
        glfwInit();
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        auto monitor = glfwGetPrimaryMonitor();
        auto mode = glfwGetVideoMode(monitor);
        window = glfwCreateWindow(mode->width, mode->height, title.c_str(), monitor, nullptr);
    }

    ~Impl() {
        glfwDestroyWindow(window);
    }

    auto shouldClose() const -> bool {
        return glfwWindowShouldClose(window);
    }

    void pollEvents() {
        glfwPollEvents();
    }

    auto createSurface(vk::Instance instance) -> vk::SurfaceKHR {
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(instance, window, nullptr, &surface);
        return static_cast<vk::SurfaceKHR>(surface);
    }

    auto getInstanceExtensions() -> std::vector<const char *> {
        uint32_t count = 0;
        auto extensions = glfwGetRequiredInstanceExtensions(&count);
        return {extensions, extensions + count};
    }
};
#else
struct Display::Impl {
    ANativeWindow* window;

    Impl(const std::string& title) {
        extern auto AndroidPlatform_getWindow() -> ANativeWindow*;

        window = AndroidPlatform_getWindow();
    }

    auto shouldClose() const -> bool {
        extern auto AndroidPlatform_shouldClose() -> bool;
        return AndroidPlatform_shouldClose();
    }

    void pollEvents() {
        extern void AndroidPlatform_pollEvents();
        AndroidPlatform_pollEvents();
    }

    auto createSurface(vk::Instance instance) -> vk::SurfaceKHR {
        const auto createInfo = vk::AndroidSurfaceCreateInfoKHR{
            .window = static_cast<ANativeWindow*>(window)
        };
        return instance.createAndroidSurfaceKHR(createInfo);
    }

    auto getInstanceExtensions() -> std::vector<const char *> {
        return {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
        };
    }
};
#endif

Display::Display(const std::string &title) {
    impl = std::make_unique<Impl>(title);
}
Display::~Display() = default;

void Display::pollEvents() {
    impl->pollEvents();
}

auto Display::shouldClose() -> bool {
    return impl->shouldClose();
}

auto Display::createSurface(vk::Instance instance) -> vk::SurfaceKHR {
    return impl->createSurface(instance);
}

auto Display::getInstanceExtensions() -> std::vector<const char *> {
    return impl->getInstanceExtensions();
}