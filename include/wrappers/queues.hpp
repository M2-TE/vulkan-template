#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>

struct Queue {
    uint32_t index;
    vk::raii::Queue queue = nullptr;
    vk::raii::CommandPool cmdPool = nullptr;
    vk::raii::CommandBuffer cmd = nullptr; // buffer for immediate submissions
    vk::raii::Semaphore timeline = nullptr; // gpu->cpu sync

    void init_sync(vk::raii::Device& device) {
        vk::CommandPoolCreateInfo poolInfo({}, index);
        cmdPool = vk::raii::CommandPool(device, poolInfo);
        vk::CommandBufferAllocateInfo buffInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 1);
        cmd = std::move(device.allocateCommandBuffers(buffInfo)[0]);
        reset_sync(device);
    }
    void reset_sync(vk::raii::Device& device) {
        vk::SemaphoreTypeCreateInfo typeInfo = vk::SemaphoreTypeCreateInfo(vk::SemaphoreType::eTimeline, 0);
        vk::SemaphoreCreateInfo semaInfo = vk::SemaphoreCreateInfo({}, &typeInfo);
        timeline = device.createSemaphore(semaInfo);
    }
};
struct Queues {
    void init(vk::raii::Device& device, vkb::Device& deviceVkb) {
        // graphics
        graphics.queue = vk::raii::Queue(device, deviceVkb.get_queue(vkb::QueueType::graphics).value());
        graphics.index = deviceVkb.get_queue_index(vkb::QueueType::graphics).value();
        // transfer (dedicated)
        auto transferVkb = deviceVkb.get_dedicated_queue(vkb::QueueType::transfer);
        transfer.queue = graphics.queue;
        if (transferVkb) transfer.queue = vk::raii::Queue(device, transferVkb.value());
        auto transferIndexVkb = deviceVkb.get_dedicated_queue_index(vkb::QueueType::transfer);
        transfer.index = graphics.index;
        if (transferIndexVkb) transfer.index = deviceVkb.get_dedicated_queue_index(vkb::QueueType::transfer).value();
        // compute (dedicated)
        auto computeVkb = deviceVkb.get_dedicated_queue(vkb::QueueType::compute);
        compute.queue = graphics.queue;
        if (computeVkb) compute.queue = vk::raii::Queue(device, computeVkb.value());
        auto computeIndexVkb = deviceVkb.get_dedicated_queue_index(vkb::QueueType::compute);
        compute.index = graphics.index;
        if (computeIndexVkb) compute.index = deviceVkb.get_dedicated_queue_index(vkb::QueueType::compute).value();

        // create command pool/buffer alongside timeline sync
        graphics.init_sync(device);
        transfer.init_sync(device);
        compute.init_sync(device);
    }

    Queue graphics;
    Queue transfer;
    Queue compute;
};