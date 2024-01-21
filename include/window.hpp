#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "wrappers/queues.hpp"

class SDL_Window;
struct Window {
    Window();
    ~Window();
    void init(vk::raii::Instance& instance, vk::DebugUtilsMessengerEXT msg);
    bool using_debug_msg();

    void imgui_init(vk::raii::Instance& instance, vk::raii::Device& device, vk::raii::PhysicalDevice& physDevice, Queues& queues, vk::Format swapchainFormat);
    static bool imgui_process_event(SDL_Event* pEvent);
    static void imgui_new_frame();
    static void imgui_draw(vk::raii::CommandBuffer& cmd);

    SDL_Window* pWindow;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMsg = nullptr;
    std::vector<const char*> extensions;
    std::string name = "Vulkan Renderer";
    vk::Extent2D extent = { 1280, 720 };
};