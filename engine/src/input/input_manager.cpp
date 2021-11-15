#include "input_manager.hpp"

struct InputManager::Impl {
};

InputManager::InputManager() {
    impl = std::make_unique<Impl>();
}

InputManager::~InputManager() = default;

void InputManager::update() {
}

void InputManager::addInput(const std::string &name, std::function<void()> action) {
    auto a = std::move(action);
}