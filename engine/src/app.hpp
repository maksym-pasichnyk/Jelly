#pragma once

struct AppMain {
    virtual ~AppMain() = default;
    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void onUpdate() = 0;
    virtual void onRender() = 0;
};