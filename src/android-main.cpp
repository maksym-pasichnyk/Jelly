#include <app.hpp>
#include <engine.hpp>
#include <input/input_system.hpp>

#include <fmt/format.h>

struct GameApp : AppMain {
    void onAttach() override {}
    void onDetach() override {}
    void onUpdate() override {}
    void onRender() override {}
};

void EngineMain(int argc, char** argv) {
    // todo: module system
    InputSystem::initialize();
    JellyEngine::initialize();
    JellyEngine::run(GameApp{});
}