#pragma once

#include <string>
#include <optional>

struct Resource;
struct ResourcePack {
    auto get(const std::string& filename) -> std::optional<Resource>;
};