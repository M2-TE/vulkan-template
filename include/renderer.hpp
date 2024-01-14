#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>
//
#include <array>
//
#include "queues.hpp"

struct Renderer {
    void init(vma::Allocator& alloc, vk::raii::Device& device, Queues& queues) {
        
        // Vulkan: create command pools and buffers
        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
            vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(queues.graphics.index);
            commandPools.emplace_back(device, poolInfo);

            vk::CommandBufferAllocateInfo bufferInfo = vk::CommandBufferAllocateInfo()
                .setCommandBufferCount(1)
                .setCommandPool(*commandPools[i])
                .setLevel(vk::CommandBufferLevel::ePrimary);
            commandBuffers.emplace_back(std::move(device.allocateCommandBuffers(bufferInfo).front()));
        }
    }

private:
    static constexpr uint32_t FRAME_OVERLAP = 2;
    std::vector<vk::raii::CommandPool> commandPools;
    std::vector<vk::raii::CommandBuffer> commandBuffers;
};