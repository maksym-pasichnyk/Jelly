#pragma once

#include <string>
#include <memory>
#include <functional>

struct InputManager;
struct InputSystem {
    friend struct JellyEngine;
    friend void EngineMain(int argc, char** argv);

    static void update();
    static void addInput(const std::string& name, std::function<void()> action);

private:
    static void initialize();

    static std::unique_ptr<InputManager> impl;
};