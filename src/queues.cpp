#include <VkBootstrap.h>
//
#include "vk_wrappers/queues.hpp"

void Queues::init(vk::raii::Device& device, vkb::Device& deviceVkb) {
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
    graphics.init(device);
    transfer.init(device);
    compute.init(device);
}