#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>
#include <fmt/base.h>
//
#include <array>
//
#include "queues.hpp"
#include "swapchain.hpp"

struct Renderer {
    void init(vk::raii::Device& device, vma::Allocator& alloc, Queues& queues) {
        
        // Vulkan: create command pools and buffers
        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
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
        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
            vk::SemaphoreCreateInfo semaInfo = vk::SemaphoreCreateInfo();
            frames[i].swapchainSemaphore = device.createSemaphore(semaInfo);
            frames[i].renderSemaphore = device.createSemaphore(semaInfo);

            vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
            frames[i].renderFence = device.createFence(fenceInfo);
        }
    }
    void draw(vk::raii::Device& device, Swapchain& swapchain, Queues& queues) {
        FrameData& frame = frames[iFrame++ % FRAME_OVERLAP];

        // wait for this frame's fence to be signaled and reset it
        while(vk::Result::eTimeout == device.waitForFences({ *frame.renderFence }, vk::True, 1000000000)) {
            fmt::println("Timeout on render fence");
        }
        device.resetFences({ *frame.renderFence});

        // acquire image from swapchain (TODO: move this into swapchain)
        auto [result, index] = swapchain.swapchain.acquireNextImage(1000000000, *frame.swapchainSemaphore);
        switch(result) {
            case vk::Result::eSuboptimalKHR: fmt::println("Suboptimal swapchain image acquisition"); break;
            case vk::Result::eTimeout: fmt::println("Timeout on swapchain image acquisition"); break;
            case vk::Result::eNotReady: fmt::println("Swapchain not yet ready"); break;
            case vk::Result::eSuccess:
            default: break;
        }

        // restart command buffer
        const vk::CommandBuffer& cmd = *frame.commandBuffer;
        cmd.reset();
        vk::CommandBufferBeginInfo cmdBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(cmdBeginInfo);

        // transition swapchain image layout (TODO: move this into swapchain)
        vk::ImageSubresourceRange range = vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0).setLevelCount(vk::RemainingMipLevels)
            .setBaseArrayLayer(0).setLayerCount(vk::RemainingArrayLayers);
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentRead)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eGeneral) // TODO: change properly
            .setSubresourceRange(range)
            .setImage(swapchain.images[index]);
        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(1)
            .setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);

        // clear swapchain image (TODO: move into swapchain)
        vk::ClearColorValue clearColor = vk::ClearColorValue(0.5f, 0.0f, 0.0f, 1.0f);
        clearColor.setFloat32({0.5f, 0.0f, 0.0f, 1.0f});
        cmd.clearColorImage(swapchain.images[index], vk::ImageLayout::eGeneral, clearColor, range);

        // transition swapchain image layout again (TODO: move this into swapchain)
        imageBarrier.setOldLayout(vk::ImageLayout::eGeneral).setNewLayout(vk::ImageLayout::ePresentSrcKHR); // TODO: set proper old layout
        depInfo.setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);
        cmd.end();

        // submit command buffer to graphics queue
        vk::SemaphoreSubmitInfo waitInfo = vk::SemaphoreSubmitInfo()
            .setSemaphore(*frame.swapchainSemaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
            .setDeviceIndex(0)
            .setValue(1);
        vk::SemaphoreSubmitInfo signalInfo = vk::SemaphoreSubmitInfo(waitInfo)
            .setSemaphore(*frame.renderSemaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);
        vk::CommandBufferSubmitInfo cmdSubmitInfo(cmd);
        vk::SubmitInfo2 submitInfo = vk::SubmitInfo2()
            .setWaitSemaphoreInfos(waitInfo)
            .setSignalSemaphoreInfos(signalInfo)
            .setCommandBufferInfos(cmdSubmitInfo);
        queues.graphics.queue.submit2(submitInfo, *frame.renderFence);

        // present swapchain image
        vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
            .setSwapchains(*swapchain.swapchain)
            .setWaitSemaphores(*frame.renderSemaphore)
            .setImageIndices(index);
        result = queues.graphics.queue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR) fmt::println("Suboptimal swapchain image presentation");
    }

private:
    struct FrameData {
        // command recording
        vk::raii::CommandPool commandPool = nullptr;
        vk::raii::CommandBuffer commandBuffer = nullptr;
        // synchronization
        vk::raii::Semaphore swapchainSemaphore = nullptr;
        vk::raii::Semaphore renderSemaphore = nullptr;
        vk::raii::Fence renderFence = nullptr;
    };
    static constexpr uint32_t FRAME_OVERLAP = 2;
    std::array<FrameData, FRAME_OVERLAP> frames;
    uint32_t iFrame = 0;
};