#include "ResourceManager.hpp"
#include "ResourcePack.hpp"
#include "Resource.hpp"

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
