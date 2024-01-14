#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>

struct Queue {
    vk::raii::Queue queue = nullptr;
    uint32_t index;
    // TODO: put a transient command pool in here
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
    }

    Queue graphics;
    Queue transfer;
    Queue compute;
};