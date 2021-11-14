#include "scene.hpp"
#include "entity.hpp"

auto Scene::createEntity() -> std::shared_ptr<Entity> {
    entities.emplace_back(std::make_shared<Entity>());
    return entities.back();
}