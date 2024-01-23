#pragma once
#include <vulkan/vulkan_raii.hpp>
//
#include "vk_wrappers/queues.hpp"
#include "vk_wrappers/image.hpp"

namespace ImGui {
    namespace backend {
        void init_sdl(SDL_Window* pWindow);
        void init_vulkan(vk::raii::Instance& instance, vk::raii::Device& device, vk::raii::PhysicalDevice& physDevice, Queues& queues, vk::Format swapchainFormat);
        bool process_event(SDL_Event* pEvent);
        void new_frame();
        void draw(vk::raii::CommandBuffer& cmd, vk::raii::ImageView& imageView, vk::ImageLayout layout, vk::Extent3D extent);
        void shutdown();
    }
}