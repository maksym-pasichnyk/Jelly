#pragma once

#include <memory>

struct ImGuiContext;
struct ImGuiLayer {
    ImGuiLayer();
    ~ImGuiLayer();

    void update(float dt);
    void begin();
    void end();
    void flush();

    void init();

private:
    struct AutoClose {
        void operator()(ImGuiContext* p);
    };

    void StyleColorsDark();
    void SetupInputBindings();

    std::unique_ptr<ImGuiContext, AutoClose> ctx;
};