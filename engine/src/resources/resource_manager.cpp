#include "resource_manager.hpp"
#include "resource_pack.hpp"
#include "resource.hpp"

void ResourceManager::emplace(std::unique_ptr<ResourcePack> &&pack) {
    packs.emplace_back(std::move(pack));
}

auto ResourceManager::get(const std::string &filename) -> std::optional<Resource> {
    for (auto& pack : packs) {
        if (auto resource = pack->get(filename)) {
            return resource;
        }
    }
    return std::nullopt;
}
