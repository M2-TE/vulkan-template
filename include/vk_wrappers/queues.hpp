#pragma once
#include <vulkan/vulkan_raii.hpp>
//
#include <functional>

// forward declare
namespace vkb { struct Device; }

struct Queue {
    // queue objects
    uint32_t index;
    vk::raii::Queue queue = nullptr;
    // oneshot command objects
    vk::raii::CommandPool cmdPool = nullptr;
    vk::raii::CommandBuffer cmd = nullptr; // buffer for immediate submissions
    vk::raii::Semaphore timeline = nullptr; // gpu->cpu sync

    void init(vk::raii::Device& device) {
        vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, index);
        cmdPool = vk::raii::CommandPool(device, poolInfo);
        vk::CommandBufferAllocateInfo buffInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 1);
        cmd = std::move(device.allocateCommandBuffers(buffInfo)[0]);
        // create timeline semaphore
        vk::SemaphoreTypeCreateInfo typeInfo = vk::SemaphoreTypeCreateInfo(vk::SemaphoreType::eTimeline, 0);
        vk::SemaphoreCreateInfo semaInfo = vk::SemaphoreCreateInfo({}, &typeInfo);
        timeline = device.createSemaphore(semaInfo);
    }

};
struct Queues {
    void init(vk::raii::Device& device, vkb::Device& deviceVkb);

    Queue graphics;
    Queue transfer;
    Queue compute;
};