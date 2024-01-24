#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "vk_wrappers/queues.hpp"

class SDL_Window;
struct Window {
    Window(int width, int height);
    ~Window();
    void init(vk::raii::Instance& instance, vk::DebugUtilsMessengerEXT msg);
    vk::Extent2D size();
    bool using_debug_msg();

    SDL_Window* pWindow;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMsg = nullptr;
    std::vector<const char*> extensions;
    std::string name = "Vulkan Renderer";
};