#pragma once

#include <string>
#include <memory>
#include <functional>

struct InputManager {
    InputManager();
    ~InputManager();

    void update();
    void addInput(const std::string& name, std::function<void()> action);

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
};