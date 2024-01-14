#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>
#include "window.hpp"

struct Swapchain {
    void init(vk::raii::PhysicalDevice& physDevice, vk::raii::Device& device, Window& window) {
        // create builder
        vkb::SwapchainBuilder swapchainBuilder(*physDevice, *device, *window.surface);
        swapchainBuilder.set_desired_extent(window.width, window.height)
            .set_desired_format(vk::SurfaceFormatKHR(vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear))
            .set_desired_present_mode((VkPresentModeKHR)vk::PresentModeKHR::eFifo)
            .add_image_usage_flags((VkImageUsageFlags)vk::ImageUsageFlagBits::eTransferDst);
        // execute builder
        vkb::Swapchain swapchainVkb = swapchainBuilder.build().value();
        std::vector<VkImage> swapchainImagesVkb = swapchainVkb.get_images().value();
        std::vector<VkImageView> swapchainImageViewsVkb = swapchainVkb.get_image_views().value();
        swapchainExtent = vk::Extent2D(swapchainVkb.extent);
        swapchainFormat = vk::Format(swapchainVkb.image_format);
        swapchain = vk::raii::SwapchainKHR(device, swapchainVkb);
        swapchainImages = swapchain.getImages();
        for (uint32_t i = 0; i < swapchainVkb.image_count; i++) swapchainImageViews.emplace_back(device, swapchainImageViewsVkb[i]);
    }
    vk::raii::SwapchainKHR swapchain = nullptr;
    std::vector<vk::raii::ImageView> swapchainImageViews;
    std::vector<vk::Image> swapchainImages;
    vk::Extent2D swapchainExtent;
    vk::Format swapchainFormat;
};