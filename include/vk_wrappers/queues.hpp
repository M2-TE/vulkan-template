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
    void cmd_immediate(vk::raii::Device& device, std::function<void(vk::raii::CommandBuffer& cmd)>&& fnc) {

        // record cmd
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        fnc(cmd);
        cmd.end();

        // submit cmd
        uint64_t waitVal = 1;
        vk::SemaphoreSubmitInfo semaSign(*timeline, waitVal, vk::PipelineStageFlagBits2::eAllCommands);
        vk::SubmitInfo2 submitInfo = vk::SubmitInfo2().setSignalSemaphoreInfos(semaSign);
        queue.submit2(submitInfo);
        vk::SemaphoreWaitInfo semaWait = vk::SemaphoreWaitInfo()
            .setSemaphores(*timeline)
            .setValues(waitVal);
        while(vk::Result::eTimeout == device.waitSemaphores(semaWait, UINT64_MAX)) {}
        // reset timeline semaphore
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