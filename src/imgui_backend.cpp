#include <vulkan/vulkan_raii.hpp>
#include <fmt/base.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
//
#include "imgui_backend.hpp"
#include "vk_wrappers/queues.hpp"

namespace ImGui {
    namespace backend {
        static vk::raii::DescriptorPool descPool = nullptr;
        static PFN_vkVoidFunction load_fnc(const char* function_name, void* user_data) {
            const vk::raii::Instance& instance = *reinterpret_cast<vk::raii::Instance*>(user_data);
            return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(*instance, function_name);
        }
        void init_sdl(SDL_Window* pWindow) {
            ImGui::CreateContext();
            ImGui_ImplSDL3_InitForVulkan(pWindow);
        }
        void init_vulkan(vk::raii::Instance& instance, vk::raii::Device& device, vk::raii::PhysicalDevice& physDevice, Queues& queues, vk::Format swapchainFormat) {
            bool success = ImGui_ImplVulkan_LoadFunctions(&load_fnc, &instance);
            if (!success) fmt::println("sdl failed to load vulkan functions");

            // create temporary descriptor pool for sdl
            std::vector<vk::DescriptorPoolSize> poolSizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
                vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)
            };
            vk::DescriptorPoolCreateInfo poolInfo = vk::DescriptorPoolCreateInfo()
                .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                .setMaxSets(1000)
                .setPoolSizes(poolSizes);
            descPool = device.createDescriptorPool(poolInfo);

            // initialize vulkan backend
            ImGui_ImplVulkan_InitInfo initInfo = {};
            initInfo.Instance = *instance;
            initInfo.PhysicalDevice = *physDevice;
            initInfo.Device = *device;
            initInfo.Queue = *queues.graphics.queue;
            initInfo.DescriptorPool = *descPool;
            initInfo.ImageCount = 3;
            initInfo.MinImageCount = initInfo.ImageCount;
            initInfo.UseDynamicRendering = true;
            initInfo.ColorAttachmentFormat = (VkFormat)swapchainFormat;
            initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            ImGui_ImplVulkan_Init(&initInfo, nullptr);;
            ImGui_ImplVulkan_CreateFontsTexture();
        }
        bool process_event(SDL_Event* pEvent) {
            return ImGui_ImplSDL3_ProcessEvent(pEvent);
        }
        void new_frame() {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
        }
        void draw(vk::raii::CommandBuffer& cmd) {
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
        }
        void shutdown() {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            descPool.clear(); 
        }
    }
}