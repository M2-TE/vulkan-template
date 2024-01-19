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
    void init(vk::raii::Device& device, vma::UniqueAllocator& alloc, Queues& queues) {
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage;
        image = Image(device, alloc, vk::Extent3D(512, 512, 1), vk::Format::eR16G16B16A16Sfloat, usage, vk::ImageAspectFlagBits::eColor);
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
        
        swapchain.present(device, image);

        // transition images to write only:
        // utils::transition_layout_rw(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eAttachmentOptimal);
        // transition images to read only:
        // utils::transition_layout_wr(cmd, swapchain.images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eReadOnlyOptimal);
    }

private:
    void shenanigans(vk::raii::Device& device, vk::raii::CommandBuffer& cmd) {
        utils::transition_layout_rw(cmd, *image.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

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
    static constexpr uint32_t FRAME_OVERLAP = 2; // double buffering
    std::array<FrameData, FRAME_OVERLAP> frames;
    uint32_t iFrame = 0;

    Image image;
    Shader shader = Shader("gradient.comp");
    vk::raii::Pipeline pipeline = nullptr;
    vk::raii::PipelineLayout layout = nullptr;
};