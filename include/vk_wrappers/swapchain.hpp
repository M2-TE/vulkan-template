#pragma once
#include <vulkan/vulkan_raii.hpp>

// forward declare
struct Window;
struct Image;
struct Queues;

struct Swapchain {
    void init(vk::raii::PhysicalDevice& physDevice, vk::raii::Device& device, Window& window, Queues& queues);
    void present(vk::raii::Device& device, Image& image, vk::raii::Semaphore& imageSema, uint64_t& semaValue);

    vk::raii::SwapchainKHR swapchain = nullptr;
    std::vector<vk::raii::ImageView> imageViews;
    std::vector<vk::Image> images;
    vk::Extent2D extent;
    vk::Format format;
    vk::Queue presentationQueue;
    bool bResizeRequested = true;

private:
    struct FrameData {
        // command recording
        vk::raii::CommandPool commandPool = nullptr;
        vk::raii::CommandBuffer commandBuffer = nullptr;
        // synchronization
        vk::raii::Semaphore swapAcquireSema = nullptr;
        vk::raii::Semaphore swapWriteSema = nullptr;
        vk::raii::Fence renderFence = nullptr;
    };
    std::vector<FrameData> frames;
    uint32_t iSyncFrame = 0;
};