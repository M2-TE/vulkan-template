#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>
#include <fmt/base.h>
//
#include "window.hpp"

struct Swapchain {
    void init(vk::raii::PhysicalDevice& physDevice, vk::raii::Device& device, Window& window) {
        // create builder
        vkb::SwapchainBuilder swapchainBuilder(*physDevice, *device, *window.surface);
        swapchainBuilder.set_desired_extent(window.extent.width, window.extent.height)
            .set_desired_format(vk::SurfaceFormatKHR(vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear))
            .set_desired_present_mode((VkPresentModeKHR)vk::PresentModeKHR::eFifo)
            .add_image_usage_flags((VkImageUsageFlags)vk::ImageUsageFlagBits::eTransferDst);
        // execute builder
        auto build = swapchainBuilder.build();
        if (!build) fmt::println("VkBootstrap error: {}", build.error().message());
        
        vkb::Swapchain swapchainVkb = build.value();
        extent = vk::Extent2D(swapchainVkb.extent);
        format = vk::Format(swapchainVkb.image_format);
        swapchain = vk::raii::SwapchainKHR(device, swapchainVkb);
        images = swapchain.getImages();
        std::vector<VkImageView> imageViewsVkb = swapchainVkb.get_image_views().value();
        for (uint32_t i = 0; i < swapchainVkb.image_count; i++) imageViews.emplace_back(device, imageViewsVkb[i]);
    }

    vk::raii::SwapchainKHR swapchain = nullptr;
    std::vector<vk::raii::ImageView> imageViews;
    std::vector<vk::Image> images;
    vk::Extent2D extent;
    vk::Format format;
};