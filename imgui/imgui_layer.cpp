#include "imgui_layer.hpp"

#include <span>
#include <imgui.h>
#include <imgui_internal.h>

void ImGuiLayer::AutoClose::operator()(ImGuiContext *p) {
    ImGui::DestroyContext(p);
}

ImGuiLayer::ImGuiLayer() {
    IMGUI_CHECKVERSION();

    ctx.reset(ImGui::CreateContext());

    auto& io = ctx->IO;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    SetupInputBindings();
    StyleColorsDark();
}

ImGuiLayer::~ImGuiLayer() = default;

void ImGuiLayer::init() {

}

void ImGuiLayer::update(float dt) {
    auto& io = ctx->IO;

    io.DeltaTime = dt;

//    io.KeyCtrl  = io.KeysDown[static_cast<int>(KeyCode::eLeftControl)] || io.KeysDown[static_cast<int>(KeyCode::eRightControl)];
//    io.KeyShift = io.KeysDown[static_cast<int>(KeyCode::eLeftShift)]   || io.KeysDown[static_cast<int>(KeyCode::eRightShift)];
//    io.KeyAlt   = io.KeysDown[static_cast<int>(KeyCode::eLeftAlt)]     || io.KeysDown[static_cast<int>(KeyCode::eRightAlt)];
//    io.KeySuper = io.KeysDown[static_cast<int>(KeyCode::eLeftSuper)]   || io.KeysDown[static_cast<int>(KeyCode::eRightSuper)];
}

void ImGuiLayer::begin() {
    ImGui::SetCurrentContext(ctx.get());
    ImGui::NewFrame();
}

void ImGuiLayer::end() {
    ImGui::Render();
    ImGui::SetCurrentContext(nullptr);
}

void ImGuiLayer::flush() {
    auto viewport = ctx->Viewports[0];
    if (viewport->DrawDataP.Valid) {
//            RenderDrawData(viewport->DrawDataP);
    }
}

void ImGuiLayer::StyleColorsDark() {
    auto& style = ctx->Style;
    auto colors = std::span(ctx->Style.Colors);

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGuiLayer::SetupInputBindings() {
//    auto& io = ctx->IO;
//    io.KeyMap[ImGuiKey_Tab] = static_cast<int>(KeyCode::eTab);
//    io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(KeyCode::eLeftArrow);
//    io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(KeyCode::eRightArrow);
//    io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(KeyCode::eUpArrow);
//    io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(KeyCode::eDownArrow);
//    io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(KeyCode::ePageUp);
//    io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(KeyCode::ePageDown);
//    io.KeyMap[ImGuiKey_Home] = static_cast<int>(KeyCode::eHome);
//    io.KeyMap[ImGuiKey_End] = static_cast<int>(KeyCode::eEnd);
//    io.KeyMap[ImGuiKey_Insert] = static_cast<int>(KeyCode::eInsert);
//    io.KeyMap[ImGuiKey_Delete] = static_cast<int>(KeyCode::eDelete);
//    io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(KeyCode::eBackspace);
//    io.KeyMap[ImGuiKey_Space] = static_cast<int>(KeyCode::eSpace);
//    io.KeyMap[ImGuiKey_Enter] = static_cast<int>(KeyCode::eEnter);
//    io.KeyMap[ImGuiKey_Escape] = static_cast<int>(KeyCode::eEscape);
//    io.KeyMap[ImGuiKey_KeyPadEnter] = static_cast<int>(KeyCode::eKeyPadEnter);
//    io.KeyMap[ImGuiKey_A] = static_cast<int>(KeyCode::eA);
//    io.KeyMap[ImGuiKey_C] = static_cast<int>(KeyCode::eC);
//    io.KeyMap[ImGuiKey_V] = static_cast<int>(KeyCode::eV);
//    io.KeyMap[ImGuiKey_X] = static_cast<int>(KeyCode::eX);
//    io.KeyMap[ImGuiKey_Y] = static_cast<int>(KeyCode::eY);
//    io.KeyMap[ImGuiKey_Z] = static_cast<int>(KeyCode::eZ);
}
