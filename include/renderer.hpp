#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>
#include <fmt/base.h>
//
#include <array>
#include <cmath>
//
#include "vk_wrappers/queues.hpp"
#include "vk_wrappers/swapchain.hpp"
#include "vk_wrappers/image.hpp"
#include "vk_wrappers/pipeline.hpp"

struct Renderer {
    void init(vk::raii::Device& device, vma::UniqueAllocator& alloc, Queues& queues, vk::Extent2D extent) {
        // create FrameData objects
        for (uint32_t i = 0; i < frames.size(); i++) {
            vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(queues.graphics.index);
            frames[i].commandPool = device.createCommandPool(poolInfo);

            vk::CommandBufferAllocateInfo bufferInfo = vk::CommandBufferAllocateInfo()
                .setCommandBufferCount(1)
                .setCommandPool(*frames[i].commandPool)
                .setLevel(vk::CommandBufferLevel::ePrimary);
            frames[i].commandBuffer = std::move(device.allocateCommandBuffers(bufferInfo).front());

            vk::SemaphoreTypeCreateInfo typeInfo = vk::SemaphoreTypeCreateInfo()
                .setInitialValue(1)
                .setSemaphoreType(vk::SemaphoreType::eTimeline);
            vk::SemaphoreCreateInfo semaInfo = vk::SemaphoreCreateInfo()
                .setPNext(&typeInfo);
            frames[i].timeline = device.createSemaphore(semaInfo);
            frames[i].timelineLast = typeInfo.initialValue;
        }

        // create image with 16 bits color depth
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage;
        image = Image(device, alloc, vk::Extent3D(extent, 1), vk::Format::eR16G16B16A16Sfloat, usage, vk::ImageAspectFlagBits::eColor);

        // create shader pipeline
        computePipe.init(device);
        computePipe.cs.write_descriptor(image, 0, 0);
    }
    void render(vk::raii::Device& device, Swapchain& swapchain, Queues& queues) {
        FrameData& frame = frames[iFrame++ % frames.size()];

        // wait for command buffer execution
        while (vk::Result::eTimeout == device.waitSemaphores(vk::SemaphoreWaitInfo({}, *frame.timeline, frame.timelineLast), UINT64_MAX)) {}
        frame.reset_timeline(device);

        // record command buffer
        vk::raii::CommandBuffer& cmd = frame.commandBuffer;
        vk::CommandBufferBeginInfo cmdBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(cmdBeginInfo);
        draw(device, cmd);
        cmd.end();

        // submit command buffer
        vk::TimelineSemaphoreSubmitInfo timelineInfo({}, ++frame.timelineLast);
        vk::SubmitInfo submitInfo = vk::SubmitInfo()
            .setPNext(&timelineInfo)
            .setSignalSemaphores(*frame.timeline)
            .setCommandBuffers(*cmd);
        queues.graphics.queue.submit(submitInfo);
        
        // present drawn image
        swapchain.present(device, image, frame.timeline, frame.timelineLast); 
    }
    
private:
    void draw(vk::raii::Device& device, vk::raii::CommandBuffer& cmd) {
        // utils::transition_layout_r_to_w(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eAttachmentOptimal);
        // utils::transition_layout_w_to_r(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eReadOnlyOptimal);
        
        image.transition_layout_r_to_w(cmd, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits2::eAllCommands, vk::PipelineStageFlagBits2::eComputeShader);
        computePipe.execute(cmd, std::ceil(image.extent.width / 16.0f), std::ceil(image.extent.height / 16.0f), 1);
    }

private:
    struct FrameData {
        void reset_timeline(vk::raii::Device& device) {
            vk::SemaphoreTypeCreateInfo typeInfo(vk::SemaphoreType::eTimeline, 0);
            vk::SemaphoreCreateInfo semaInfo({}, &typeInfo);
            timeline = device.createSemaphore(semaInfo);
            timelineLast = typeInfo.initialValue;
        }
        vk::raii::CommandPool commandPool = nullptr;
        vk::raii::CommandBuffer commandBuffer = nullptr;
        vk::raii::Semaphore timeline = nullptr;
        uint64_t timelineLast;
    };
    std::array<FrameData, 2> frames; // double buffering
    uint32_t iFrame = 0;

    Image image;
    Pipelines::Compute computePipe = {"gradient.comp"};
};