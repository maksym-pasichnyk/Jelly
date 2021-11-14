#pragma once

#include <memory>
#include <vector>
#include <shared_library.hpp>

struct Entity;

struct Scene {
    friend struct AppTest;

    auto createEntity() -> std::shared_ptr<Entity>;

private:
    explicit Scene(SharedLibrary library) : library(std::move(library)) {}

    SharedLibrary library;
    std::vector<std::shared_ptr<Entity>> entities;
};