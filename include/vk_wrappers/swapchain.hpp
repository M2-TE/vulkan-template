#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>
#include <fmt/base.h>
//
#include "window.hpp"
#include "image.hpp"
#include "utils.hpp"
#include "imgui_impl.hpp"

struct Swapchain {
    void init(vk::raii::PhysicalDevice& physDevice, vk::raii::Device& device, Window& window, Queues& queues) {
        bResizeRequested = false;

        // VkBoostrap: build swapchain
        vkb::SwapchainBuilder swapchainBuilder(*physDevice, *device, *window.surface);
        swapchainBuilder.set_desired_extent(window.size().width, window.size().height)
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
            frames[i].swapAcquireSema = device.createSemaphore(semaInfo);
            frames[i].swapWriteSema = device.createSemaphore(semaInfo);

            vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
            frames[i].renderFence = device.createFence(fenceInfo);
        }
    }
    void present(vk::raii::Device& device, Image& image, vk::raii::Semaphore& imageSema, uint64_t& semaValue) {
        FrameData& frame = frames[iSyncFrame++ % frames.size()];
        vk::Result result;
        uint32_t index; // index into swapchain image array

        // wait for this frame's fence to be signaled and reset it
        result = vk::Result::eTimeout;
        while (vk::Result::eTimeout == device.waitForFences(*frame.renderFence, vk::True, UINT64_MAX));
        device.resetFences({ *frame.renderFence });

        // acquire image from swapchain
        result = vk::Result::eTimeout;
        while (vk::Result::eTimeout == result) std::tie(result, index) = swapchain.acquireNextImage(UINT64_MAX, *frame.swapAcquireSema);

        // restart command buffer
        vk::raii::CommandBuffer& cmd = frame.commandBuffer;
        vk::CommandBufferBeginInfo cmdBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(cmdBeginInfo);

        // transition image layouts for upcoming blit
        utils::transition_layout_rw(cmd, images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits2::eAllCommands, vk::PipelineStageFlagBits2::eBlit);
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
        
        // draw ImGui UI directly onto swapchain image
        utils::transition_layout_rw(cmd, images[index], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eAttachmentOptimal,
            vk::PipelineStageFlagBits2::eBlit, vk::PipelineStageFlagBits2::eAllGraphics);
        ImGui::backend::draw(cmd, imageViews[index], vk::ImageLayout::eAttachmentOptimal, extent);

        // finalize swapchain image
        utils::transition_layout_wr(cmd, images[index], vk::ImageLayout::eAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits2::eBlit, vk::PipelineStageFlagBits2::eAllCommands);
        cmd.end();

        // submit command buffer to graphics queue
        std::array<vk::SemaphoreSubmitInfo, 2> waitInfos = {
            vk::SemaphoreSubmitInfo()
                .setSemaphore(*imageSema)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setValue(semaValue),
            vk::SemaphoreSubmitInfo()
                .setSemaphore(*frame.swapAcquireSema)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        };
        std::array<vk::SemaphoreSubmitInfo, 2> signInfos = {
            vk::SemaphoreSubmitInfo()
                .setSemaphore(*imageSema)
                .setStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setValue(++semaValue),
            vk::SemaphoreSubmitInfo()
                .setSemaphore(*frame.swapWriteSema)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        };
        vk::CommandBufferSubmitInfo cmdSubmitInfo(*cmd);
        vk::SubmitInfo2 submitInfo = vk::SubmitInfo2()
            .setWaitSemaphoreInfos(waitInfos)
            .setSignalSemaphoreInfos(signInfos)
            .setCommandBufferInfos(cmdSubmitInfo);
        presentationQueue.submit2(submitInfo, *frame.renderFence);

        // present swapchain image
        vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
            .setSwapchains(*swapchain)
            .setWaitSemaphores(*frame.swapWriteSema)
            .setImageIndices(index);
        try { result = presentationQueue.presentKHR(presentInfo); }
        catch (vk::OutOfDateKHRError) { bResizeRequested = true; }
    }

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