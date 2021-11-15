#include "resource_pack.hpp"
#include "resource.hpp"

#if _WIN32
#else
#include <android/asset_manager.h>
#endif

auto ResourcePack::get(const std::string& filename) -> std::optional<Resource> {
#if _WIN32
    return std::nullopt;
#else
    extern auto AndroidPlatform_getAssets() -> AAssetManager*;

    auto assets = AndroidPlatform_getAssets();
    auto asset = AAssetManager_open(assets, filename.c_str(), AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        return std::nullopt;
    }

    auto resource = Resource(static_cast<size_t>(AAsset_getLength(asset)));
    AAsset_read(asset, resource.bytes_for_write(), resource.size());
    AAsset_close(asset);
    return resource;
#endif
}