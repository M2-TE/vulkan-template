#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>
#include <VkBootstrap.h>
#include <SDL3/SDL_events.h>
#include <fmt/base.h>
//
#include <chrono>
#include <thread>
#include <vector>
//
#include "window.hpp"
#include "swapchain.hpp"
#include "queues.hpp"
#include "renderer.hpp"

struct Engine {
    Engine() {
        // Vulkan: dynamic dispatcher init 1/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        // VkBootstrap: create vulkan instance
        vkb::InstanceBuilder builder;
        builder.set_app_name(window.name.c_str())
            .enable_extensions(window.extensions)
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0);
        if (window.use_debug_msg()) builder.request_validation_layers();
        vkb::Instance instanceVkb = builder.build().value();
        instance = vk::raii::Instance(context, instanceVkb);

        // Vulkan: dynamic dispatcher init 2/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
        
        // SDL: create vulkan surface
        window.init(instance, instanceVkb.debug_messenger);

        // VkBootstrap: select/create device
        vkb::PhysicalDeviceSelector selector(instanceVkb, *window.surface);
        selector.set_minimum_version(1, 3)
            .set_required_features_13(vk::PhysicalDeviceVulkan13Features()
                .setDynamicRendering(true)
                .setSynchronization2(true))
            .set_required_features_12(vk::PhysicalDeviceVulkan12Features()
                .setBufferDeviceAddress(true)
                .setDescriptorIndexing(true));
        vkb::PhysicalDevice physicalDeviceVkb = selector.select().value();
        vkb::Device deviceVkb = vkb::DeviceBuilder(physicalDeviceVkb).build().value();
        physDevice = vk::raii::PhysicalDevice(instance, physicalDeviceVkb);
        device = vk::raii::Device(physDevice, deviceVkb);
        
        // Vulkan: dynamic dispatcher init 3/3
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

        // VMA: create allocator
        vma::VulkanFunctions vulkanFuncs(VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr, VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr);
        vma::AllocatorCreateInfo allocInfo = vma::AllocatorCreateInfo()
            .setVulkanApiVersion(vk::ApiVersion13)
            .setPVulkanFunctions(&vulkanFuncs)
            .setPhysicalDevice(*physDevice)
            .setInstance(*instance)
            .setDevice(*device);
        alloc = vma::createAllocator(allocInfo);

        // VkBootstrap: create swapchain
        swapchain.init(physDevice, device, window);
        // VkBootstrap: create command queues
        queues.init(device, deviceVkb);
        // Vulkan: create command buffers
        renderer.init(alloc, device, queues);
    }
    void run() {
        bRunning = true;
        bRendering = true;
        while(bRunning){
            SDL_Event event;
            while (SDL_PollEvent(&event)) handle_event(event);

            if (bRendering) draw();
            else std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    void handle_event(SDL_Event& event) {
        switch (event.type) {
            case SDL_EventType::SDL_EVENT_QUIT: bRunning = false; break;
            case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED: bRendering = false; break;
            case SDL_EventType::SDL_EVENT_WINDOW_RESTORED: bRendering = true; break;
            default: break;
        }
    }
    void draw() {
        framecount++;
    }

private:
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::PhysicalDevice physDevice = nullptr;
    vk::raii::Device device = nullptr;
    vma::Allocator alloc;
    Window window;
    Swapchain swapchain;
    Queues queues;
    Renderer renderer;

    bool bRunning;
    bool bRendering;
    uint64_t framecount = 0;
};