#pragma once

#include <vector>
#include <memory>
#include <string>
#include <optional>

struct Resource;
struct ResourcePack;
struct ResourceManager {
    void emplace(std::unique_ptr<ResourcePack>&& pack);
    auto get(const std::string& filename) -> std::optional<Resource>;

private:
    std::vector<std::unique_ptr<ResourcePack>> packs;
};