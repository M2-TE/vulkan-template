#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>
#include <fmt/base.h>
//
#include <array>
//
#include "queues.hpp"
#include "swapchain.hpp"
#include "image.hpp"
#include "utils.hpp"

struct Renderer {
    void init(vk::raii::Device& device, vma::UniqueAllocator& alloc, Queues& queues) {
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
        
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage;
        image = Image(device, alloc, vk::Extent3D(512, 512, 1), vk::Format::eR16G16B16A16Sfloat, usage, vk::ImageAspectFlagBits::eColor);
    }
    void draw(vk::raii::Device& device, Swapchain& swapchain, Queues& queues) {
        FrameData& frame = frames[iFrame++ % FRAME_OVERLAP];

        // wait for this frame's fence to be signaled and reset it
        while(vk::Result::eTimeout == device.waitForFences({ *frame.renderFence }, vk::True, UINT64_MAX)) {}
        device.resetFences({ *frame.renderFence});

        // acquire image from swapchain
        auto [result, index] = swapchain.swapchain.acquireNextImage(UINT64_MAX, *frame.swapchainSemaphore);
        switch(result) {
            case vk::Result::eSuboptimalKHR: fmt::println("Suboptimal swapchain image acquisition"); break;
            case vk::Result::eTimeout: fmt::println("Timeout on swapchain image acquisition"); break;
            case vk::Result::eNotReady: fmt::println("Swapchain not yet ready"); break;
            case vk::Result::eSuccess:
            default: break;
        }

        // restart command buffer
        vk::raii::CommandBuffer& cmd = frame.commandBuffer;
        cmd.reset();
        vk::CommandBufferBeginInfo cmdBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(cmdBeginInfo);

        shenanigans(device, cmd);

        // clear swapchain image
        utils::transition_layout_rw(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        vk::ClearColorValue clearColor = vk::ClearColorValue(0.5f, 0.0f, 0.0f, 1.0f);
        cmd.clearColorImage(swapchain.images[index], vk::ImageLayout::eTransferDstOptimal, clearColor, utils::default_subresource_range());

        // TODO: render geometry

        // transition images to write only:
        // utils::transition_layout_rw(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eAttachmentOptimal);
        // transition images to read only:
        // utils::transition_layout_wr(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eReadOnlyOptimal);

        // finalize swapchain image
        utils::transition_layout_wr(cmd, swapchain.images[index], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);
        cmd.end();

        // submit command buffer to graphics queue
        vk::SemaphoreSubmitInfo waitInfo = vk::SemaphoreSubmitInfo()
            .setSemaphore(*frame.swapchainSemaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDeviceIndex(0)
            .setValue(1);
        vk::SemaphoreSubmitInfo signalInfo = vk::SemaphoreSubmitInfo(waitInfo)
            .setSemaphore(*frame.renderSemaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        vk::CommandBufferSubmitInfo cmdSubmitInfo(*cmd);
        vk::SubmitInfo2 submitInfo = vk::SubmitInfo2()
            .setWaitSemaphoreInfos(waitInfo)
            .setCommandBufferInfos(cmdSubmitInfo)
            .setSignalSemaphoreInfos(signalInfo);
        queues.graphics.queue.submit2(submitInfo, *frame.renderFence); // could use transfer queue

        // present swapchain image
        vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
            .setSwapchains(*swapchain.swapchain)
            .setWaitSemaphores(*frame.renderSemaphore)
            .setImageIndices(index);
        result = queues.graphics.queue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR) fmt::println("Suboptimal swapchain image presentation");
    }

private:
    void shenanigans(vk::raii::Device& device, vk::raii::CommandBuffer& cmd) {
        return;
        
        vk::BlitImageInfo2 blitInfo = vk::BlitImageInfo2()
            .setRegions(vk::ImageBlit2()
                .setSrcOffsets({ vk::Offset3D(), vk::Offset3D(image.extent.width, image.extent.height, 1) })
                .setDstOffsets({ vk::Offset3D(), vk::Offset3D(image.extent.width, image.extent.height, 1) })
                .setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
                .setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)))
            .setDstImage(*image.image)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcImage(*image.image)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eLinear);
        cmd.blitImage2(blitInfo);
        // TODO: use copyImage() instead when img and swapchain image match in size
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

    Image image;
};