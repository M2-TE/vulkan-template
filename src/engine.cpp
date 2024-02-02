#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>
#include <SDL3/SDL_events.h>
#include <fmt/base.h>
//
#include "engine.hpp"
#include "window.hpp"
#include "renderer.hpp"
#include "vk_wrappers/imgui_impl.hpp"
#include "vk_wrappers/swapchain.hpp"
#include "vk_wrappers/queues.hpp"

Engine::Engine() {
    // Vulkan: dynamic dispatcher init 1/3
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    // VkBootstrap: create vulkan instance
    vkb::InstanceBuilder builder;
    builder.set_app_name(window.name.c_str())
        .enable_extensions(window.extensions)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0);
    if (window.using_debug_msg()) builder.request_validation_layers();
    auto instanceBuild = builder.build();
    if (!instanceBuild) fmt::println("VkBootstrap error: {}", instanceBuild.error().message());
    vkb::Instance instanceVkb = instanceBuild.value();
    instance = vk::raii::Instance(context, instanceVkb);

    // Vulkan: dynamic dispatcher init 2/3
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    // SDL: create vulkan surface
    window.init(instance, instanceVkb.debug_messenger);

    // VkBootstrap: select physical device
    vkb::PhysicalDeviceSelector selector(instanceVkb, *window.surface);
    selector.set_minimum_version(1, 3)
        .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .add_required_extension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME)
        //.add_required_extension("VK_KHR_dynamic_rendering_local_read")
        .set_required_features_11(vk::PhysicalDeviceVulkan11Features())
        .set_required_features_12(vk::PhysicalDeviceVulkan12Features()
            .setTimelineSemaphore(true)
            .setBufferDeviceAddress(true)
            .setDescriptorIndexing(true))
        .set_required_features_13(vk::PhysicalDeviceVulkan13Features()
            .setDynamicRendering(true)
            .setSynchronization2(true));
    auto deviceSelection = selector.select();
    if (!deviceSelection) fmt::println("VkBootstrap error: {}", deviceSelection.error().message());
    vkb::PhysicalDevice physicalDeviceVkb = deviceSelection.value();
    physDevice = vk::raii::PhysicalDevice(instance, physicalDeviceVkb);

    // VkBootstrap: create device
    auto deviceBuilder = vkb::DeviceBuilder(physicalDeviceVkb).build();
    if (!deviceBuilder) fmt::println("VkBootstrap error: {}", deviceBuilder.error().message());
    vkb::Device deviceVkb = deviceBuilder.value();
    device = vk::raii::Device(physDevice, deviceVkb);

    // Vulkan: dynamic dispatcher init 3/3
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

    // VMA: create allocator
    vma::VulkanFunctions vulkanFuncs(VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr, VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr);
    vma::AllocatorCreateInfo allocInfo = vma::AllocatorCreateInfo()
        .setFlags(vma::AllocatorCreateFlagBits::eBufferDeviceAddress | vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation)
        .setVulkanApiVersion(vk::ApiVersion13)
        .setPVulkanFunctions(&vulkanFuncs)
        .setPhysicalDevice(*physDevice)
        .setInstance(*instance)
        .setDevice(*device);
    alloc = vma::createAllocatorUnique(allocInfo);

    // create command queues
    queues.init(device, deviceVkb);
    // create swapchain
    swapchain.init(physDevice, device, window, queues);
    // create render pipelines
    renderer.init(device, alloc, queues, vk::Extent2D(window.size()));
    // initialize imgui backend
    ImGui::backend::init_sdl(window.pWindow);
    ImGui::backend::init_vulkan(instance, device, physDevice, queues, swapchain.format);
}