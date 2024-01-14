#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace utils {
    inline vk::ImageSubresourceRange& default_subresource_range() {
        static vk::ImageSubresourceRange range = vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0).setLevelCount(vk::RemainingMipLevels)
            .setBaseArrayLayer(0).setLayerCount(vk::RemainingArrayLayers);
        return range;
    }

    inline void transition_layout(vk::raii::CommandBuffer& cmd, vk::raii::Image& image, vk::ImageLayout layoutOld, vk::ImageLayout layoutNew) {
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands) // TODO: adjust
            .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite) // TODO: adjust
            .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands) // TODO: adjust
            .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentRead) // TODO: adjust
            .setOldLayout(layoutOld)
            .setNewLayout(layoutNew)
            .setSubresourceRange(default_subresource_range())
            .setImage(*image);
        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(1)
            .setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);
    }
    inline void transition_layout(vk::raii::CommandBuffer& cmd, vk::Image& image, vk::ImageLayout layoutOld, vk::ImageLayout layoutNew) {
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands) // TODO: adjust
            .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite) // TODO: adjust
            .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands) // TODO: adjust
            .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentRead) // TODO: adjust
            .setOldLayout(layoutOld)
            .setNewLayout(layoutNew)
            .setSubresourceRange(default_subresource_range())
            .setImage(image);
        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(1)
            .setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);
    }
};