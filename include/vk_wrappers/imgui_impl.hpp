#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <SDL3/SDL_events.h>
#include <imgui.h>
//
#include "vk_wrappers/queues.hpp"
#include "vk_wrappers/image.hpp"

namespace ImGui {
    namespace frontend {
        static void display_fps() {
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::Begin("FPS_Overlay", nullptr, ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_NoDocking
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoFocusOnAppearing
                | ImGuiWindowFlags_AlwaysAutoResize
                | ImGuiWindowFlags_NoNav
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoMouseInputs);
            ImGui::Text("%.1f fps", ImGui::GetIO().Framerate);
            ImGui::Text("%.1f ms", ImGui::GetIO().DeltaTime * 1000.0f);
            ImGui::End();
        }
    }
    namespace backend {
        void init_sdl(SDL_Window* pWindow);
        void init_vulkan(vk::raii::Instance& instance, vk::raii::Device& device, vk::raii::PhysicalDevice& physDevice, Queues& queues, vk::Format swapchainFormat);
        bool process_event(SDL_Event* pEvent);
        void new_frame();
        void draw(vk::raii::CommandBuffer& cmd, vk::raii::ImageView& imageView, vk::ImageLayout layout, vk::Extent2D extent);
        void shutdown();
    }
}