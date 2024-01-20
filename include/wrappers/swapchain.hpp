#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>
#include <fmt/base.h>
//
#include "window.hpp"
#include "image.hpp"
#include "utils.hpp"

struct Swapchain {
    void init(vk::raii::PhysicalDevice& physDevice, vk::raii::Device& device, Window& window, Queues& queues) {
        // VkBoostrap: build swapchain
        vkb::SwapchainBuilder swapchainBuilder(*physDevice, *device, *window.surface);
        swapchainBuilder.set_desired_extent(window.extent.width, window.extent.height)
            .set_desired_format(vk::SurfaceFormatKHR(vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear))
            .set_desired_present_mode((VkPresentModeKHR)vk::PresentModeKHR::eFifo)
            .add_image_usage_flags((VkImageUsageFlags)vk::ImageUsageFlagBits::eTransferDst);
        auto build = swapchainBuilder.build();
        if (!build) fmt::println("VkBootstrap error: {}", build.error().message());
        vkb::Swapchain swapchainVkb = build.value();
        extent = vk::Extent2D(swapchainVkb.extent);
        format = vk::Format(swapchainVkb.image_format);
        swapchain = vk::raii::SwapchainKHR(device, swapchainVkb);
        images = swapchain.getImages();
        std::vector<VkImageView> imageViewsVkb = swapchainVkb.get_image_views().value();
        for (uint32_t i = 0; i < swapchainVkb.image_count; i++) imageViews.emplace_back(device, imageViewsVkb[i]);

        // Vulkan: create command pools and buffers
        presentationQueue = *queues.graphics.queue;
        frames.resize(swapchainVkb.image_count);
        for (uint32_t i = 0; i < swapchainVkb.image_count; i++) {
            vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(queues.graphics.index);
            frames[i].commandPool = device.createCommandPool(poolInfo);

            vk::CommandBufferAllocateInfo bufferInfo = vk::CommandBufferAllocateInfo()
                .setCommandBufferCount(1)
                .setCommandPool(*frames[i].commandPool)
                .setLevel(vk::CommandBufferLevel::ePrimary);
            frames[i].commandBuffer = std::move(device.allocateCommandBuffers(bufferInfo).front());
        }

        // Vulkan:: create synchronization objects
        for (uint32_t i = 0; i < swapchainVkb.image_count; i++) {
            vk::SemaphoreCreateInfo semaInfo = vk::SemaphoreCreateInfo();
            frames[i].swapchainSemaphore = device.createSemaphore(semaInfo);
            frames[i].renderSemaphore = device.createSemaphore(semaInfo);

            vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
            frames[i].renderFence = device.createFence(fenceInfo);
        }
    }
    void present(vk::raii::Device& device, Image& image) {
        FrameData& frame = frames[iSyncFrame++ % frames.size()];

        // wait for this frame's fence to be signaled and reset it
        while (vk::Result::eTimeout == device.waitForFences({ *frame.renderFence }, vk::True, UINT64_MAX)) {}
        device.resetFences({ *frame.renderFence });

        // acquire image from swapchain
        auto [result, index] = swapchain.acquireNextImage(UINT64_MAX, *frame.swapchainSemaphore);
        switch (result) {
            case vk::Result::eSuboptimalKHR: fmt::println("Suboptimal swapchain image acquisition"); break;
            case vk::Result::eTimeout: fmt::println("Timeout on swapchain image acquisition"); break;
            case vk::Result::eNotReady: fmt::println("Swapchain not yet ready"); break;
            case vk::Result::eSuccess:
            default: break;
        }

        // restart command buffer
        vk::raii::CommandBuffer& cmd = frame.commandBuffer;
        vk::CommandBufferBeginInfo cmdBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(cmdBeginInfo);

        // clear swapchain image
        utils::transition_layout_rw(cmd, images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits2::eAllCommands, vk::PipelineStageFlagBits2::eClear);
        vk::ClearColorValue clearColor = vk::ClearColorValue(0.5f, 0.0f, 0.0f, 1.0f);
        cmd.clearColorImage(images[index], vk::ImageLayout::eTransferDstOptimal, clearColor, utils::default_subresource_range());

        // transition input image to readable layout
        image.transition_layout_wr(cmd, vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits2::eAllCommands, vk::PipelineStageFlagBits2::eBlit);

        // copy input image to swapchain image
        vk::BlitImageInfo2 blitInfo = vk::BlitImageInfo2()
            .setRegions(vk::ImageBlit2()
                .setSrcOffsets({ vk::Offset3D(), vk::Offset3D(image.extent.width, image.extent.height, 1) })
                .setDstOffsets({ vk::Offset3D(), vk::Offset3D(extent.width, extent.height, 1)})
                .setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
                .setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)))
            .setSrcImage(*image.image)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImage(images[index])
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eLinear);
        cmd.blitImage2(blitInfo);

        // finalize swapchain image
        utils::transition_layout_wr(cmd, images[index], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits2::eBlit, vk::PipelineStageFlagBits2::eAllCommands);
        cmd.end();

        // submit command buffer to graphics queue
        vk::SemaphoreSubmitInfo swapchainWaitInfo = vk::SemaphoreSubmitInfo()
            .setSemaphore(*frame.swapchainSemaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDeviceIndex(0)
            .setValue(1);
        vk::SemaphoreSubmitInfo renderSignalInfo = vk::SemaphoreSubmitInfo(swapchainWaitInfo)
            .setSemaphore(*frame.renderSemaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        vk::CommandBufferSubmitInfo cmdSubmitInfo(*cmd);
        vk::SubmitInfo2 submitInfo = vk::SubmitInfo2()
            .setWaitSemaphoreInfos(swapchainWaitInfo)
            .setCommandBufferInfos(cmdSubmitInfo)
            .setSignalSemaphoreInfos(renderSignalInfo);
        presentationQueue.submit2(submitInfo, *frame.renderFence);

        // present swapchain image
        vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
            .setSwapchains(*swapchain)
            .setWaitSemaphores(*frame.renderSemaphore)
            .setImageIndices(index);
        result = presentationQueue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR) fmt::println("Suboptimal swapchain image presentation");
    }

    vk::raii::SwapchainKHR swapchain = nullptr;
    std::vector<vk::raii::ImageView> imageViews;
    std::vector<vk::Image> images;
    vk::Extent2D extent;
    vk::Format format;
    vk::Queue presentationQueue;

    // presentation synchronization
    struct FrameData {
        // command recording
        vk::raii::CommandPool commandPool = nullptr;
        vk::raii::CommandBuffer commandBuffer = nullptr;
        // synchronization
        vk::raii::Semaphore swapchainSemaphore = nullptr;
        vk::raii::Semaphore renderSemaphore = nullptr;
        vk::raii::Fence renderFence = nullptr;
    };
    std::vector<FrameData> frames;
    uint32_t iSyncFrame = 0;
};