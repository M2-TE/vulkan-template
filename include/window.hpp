#pragma once

struct Window {
    Window();
    ~Window();
    void init(vk::raii::Instance& instance, vk::DebugUtilsMessengerEXT msg);
    bool use_debug_msg();

    SDL_Window* pWindow;
    vk::raii::SurfaceKHR surface = nullptr;
    std::vector<const char*> extensions;
    std::string name = "Vulkan Renderer";
    int32_t width = 1280, height = 720;
    vk::raii::DebugUtilsMessengerEXT debugMsg = nullptr;
};