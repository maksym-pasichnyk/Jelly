#pragma once

#include <string>
#include <memory>
#include <optional>

struct AppMain;
struct JellyEngine {
    friend void EngineMain(int argc, char** argv);

private:
    JellyEngine();
    ~JellyEngine();

    static void initialize();
    static void run(AppMain& app);

    static void run(AppMain&& app) {
        run(app);
    }

private:
    struct Impl;

    static std::unique_ptr<Impl> impl;
};
