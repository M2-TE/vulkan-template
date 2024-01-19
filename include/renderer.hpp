#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>
#include <fmt/base.h>
//
#include <array>
//
#include "utils.hpp"
#include "wrappers/queues.hpp"
#include "wrappers/swapchain.hpp"
#include "wrappers/image.hpp"
#include "wrappers/shader.hpp"

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
        }

        // create image with 16 bits color depth
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage;
        image = Image(device, alloc, vk::Extent3D(extent, 1), vk::Format::eR16G16B16A16Sfloat, usage, vk::ImageAspectFlagBits::eColor);

        // create shader pipeline (TODO: move more of this to pipeline.hpp or something)
        shader.init(device);
        shader.create_descriptors(device, image);
        vk::PipelineLayoutCreateInfo layoutInfo = vk::PipelineLayoutCreateInfo()
            .setSetLayouts(*shader.descSetLayout);
        layout = device.createPipelineLayout(layoutInfo);
        vk::PipelineShaderStageCreateInfo stageInfo = vk::PipelineShaderStageCreateInfo()
            .setModule(*shader.shader)
            .setStage(vk::ShaderStageFlagBits::eCompute)
            .setPName("main"); // is this needed?
        vk::ComputePipelineCreateInfo pipeInfo = vk::ComputePipelineCreateInfo()
            .setLayout(*layout)
            .setStage(stageInfo);
        pipeline = device.createComputePipeline(nullptr, pipeInfo);
    }
    void draw(vk::raii::Device& device, Swapchain& swapchain, Queues& queues) {
        FrameData& frame = frames[iFrame++ % frames.size()];

        // wait for command buffer execution
        uint64_t signVal = 1;
        while (vk::Result::eTimeout == device.waitSemaphores(vk::SemaphoreWaitInfo().setSemaphores(*frame.timeline).setValues(signVal), UINT64_MAX)) {}

        // reset semaphore
        vk::SemaphoreTypeCreateInfo typeInfo = vk::SemaphoreTypeCreateInfo()
            .setInitialValue(0)
            .setSemaphoreType(vk::SemaphoreType::eTimeline);
        vk::SemaphoreCreateInfo semaInfo = vk::SemaphoreCreateInfo()
            .setPNext(&typeInfo);
        frame.timeline = device.createSemaphore(semaInfo);

        // record command buffer
        vk::raii::CommandBuffer& cmd = frame.commandBuffer;
        cmd.reset();
        vk::CommandBufferBeginInfo cmdBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(cmdBeginInfo);
        draw(device, cmd);
        cmd.end();

        // submit command buffer
        vk::TimelineSemaphoreSubmitInfo timelineInfo = vk::TimelineSemaphoreSubmitInfo()
            .setSignalSemaphoreValues(signVal);
        vk::SubmitInfo submitInfo = vk::SubmitInfo()
            .setPNext(&timelineInfo)
            .setSignalSemaphores(*frame.timeline)
            .setCommandBuffers(*cmd);
        queues.graphics.queue.submit(submitInfo);
        
        //shenanigans(device, cmd);
        swapchain.present(device, image);
    }

private:
    void draw(vk::raii::Device& device, vk::raii::CommandBuffer& cmd) {
        // transition images to write only:
        // utils::transition_layout_rw(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eAttachmentOptimal);
        // transition images to read only:
        // utils::transition_layout_wr(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eReadOnlyOptimal);

        image.transition_layout_rw(cmd, vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits2::eAllCommands, vk::PipelineStageFlagBits2::eComputeShader);

        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, { *shader.descSet }, {});
        cmd.dispatch(std::ceil(image.extent.width / 16.0), std::ceil(image.extent.height / 16.0), 1);
    }
private:
    struct FrameData {
        // command recording
        vk::raii::CommandPool commandPool = nullptr;
        vk::raii::CommandBuffer commandBuffer = nullptr;
        // synchronization
        vk::raii::Semaphore timeline = nullptr;
    };
    std::array<FrameData, 2> frames; // double buffering
    uint32_t iFrame = 0;

    Image image;
    Shader shader = Shader("gradient.comp");
    vk::raii::Pipeline pipeline = nullptr;
    vk::raii::PipelineLayout layout = nullptr;
};