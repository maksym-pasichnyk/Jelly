#include "input_system.hpp"
#include "input_manager.hpp"

std::unique_ptr<InputManager> InputSystem::impl;

void InputSystem::initialize() {
    impl = std::make_unique<InputManager>();
}

void InputSystem::update() {
    impl->update();
}

void InputSystem::addInput(const std::string &name, std::function<void()> action) {
    impl->addInput(name, std::move(action));
}
