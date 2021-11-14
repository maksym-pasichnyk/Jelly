#pragma once

#include <optional>
#include <string>
#include <memory>

#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __QNXNTO__ ) || defined( __Fuchsia__ )
#include <dlfcn.h>
#elif defined( _WIN32 )
#include <libloaderapi.h>
#else
#error unsupported platform
#endif

struct SharedLibrary {
    static auto open(const std::string &name) noexcept -> std::optional<SharedLibrary> {
#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __QNXNTO__ ) || defined( __Fuchsia__ )
        auto lib = dlopen(name.c_str(), RTLD_NOW | RTLD_LOCAL);
#elif defined( _WIN32 )
        auto lib = LoadLibraryA(name.c_str());
#else
#error unsupported platform
#endif
        if (lib == nullptr) {
            return std::nullopt;
        }
        return SharedLibrary(lib);
    }

    template<typename T>
    auto get(const char* function) const noexcept -> T {
#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __QNXNTO__ ) || defined( __Fuchsia__ )
        return (T)dlsym(m_library.get(), function);
#elif defined( _WIN32 )
        return (T)GetProcAddress(m_library.get(), function);
#else
#error unsupported platform
#endif
    }

private:
    struct AutoClose {
#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __QNXNTO__ ) || defined( __Fuchsia__ )
        void operator()(void* handle) noexcept {
            dlclose(handle);
#elif defined( _WIN32 )
        void operator()(HINSTANCE handle) noexcept {
            FreeLibrary(handle);
#else
#error unsupported platform
#endif
        }
    };

#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __QNXNTO__ ) || defined( __Fuchsia__ )
    explicit SharedLibrary(void* handle) noexcept {
        m_library.reset(handle);
    }
    std::unique_ptr<void, AutoClose> m_library;
#elif defined( _WIN32 )
    explicit SharedLibrary(HINSTANCE handle) noexcept {
        m_library.reset(handle);
    }
    std::unique_ptr<std::remove_pointer_t<HINSTANCE>, AutoClose> m_library;
#else
#error unsupported platform
#endif
};