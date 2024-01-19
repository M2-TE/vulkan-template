#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace utils {
    inline vk::ImageSubresourceRange default_subresource_range() {
        return vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers);
    }

    // transition into a write layout (from read)
    inline void transition_layout_rw(vk::raii::CommandBuffer& cmd, vk::Image& image, 
            vk::ImageLayout layoutOld, vk::ImageLayout layoutNew,
            vk::PipelineStageFlagBits2 srcStage, vk::PipelineStageFlagBits2 dstStage) {
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(srcStage)
            .setSrcAccessMask(vk::AccessFlagBits2::eMemoryRead)
            .setDstStageMask(dstStage)
            .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite)
            .setOldLayout(layoutOld)
            .setNewLayout(layoutNew)
            .setSubresourceRange(default_subresource_range())
            .setImage(image);
        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(1)
            .setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);
    }
    // transition into a read layout (from write)
    inline void transition_layout_wr(vk::raii::CommandBuffer& cmd, vk::Image& image, 
            vk::ImageLayout layoutOld, vk::ImageLayout layoutNew,
            vk::PipelineStageFlagBits2 srcStage, vk::PipelineStageFlagBits2 dstStage) {
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(srcStage)
            .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
            .setDstStageMask(dstStage)
            .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead)
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