#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "vk_wrappers/queues.hpp"

class SDL_Window;
struct Window {
    Window();
    ~Window();
    void init(vk::raii::Instance& instance, vk::DebugUtilsMessengerEXT msg);
    bool using_debug_msg();

    SDL_Window* pWindow;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMsg = nullptr;
    std::vector<const char*> extensions;
    std::string name = "Vulkan Renderer";
    vk::Extent2D extent = { 1280, 720 };
};