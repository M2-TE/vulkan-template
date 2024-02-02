#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>

struct Image {
    Image() = default;
    Image(vk::raii::Device& device, vma::UniqueAllocator& alloc, 
            vk::Extent3D extent, vk::Format format, 
            vk::ImageUsageFlags usage, vk::ImageAspectFlags aspects)
                : extent(extent), format(format) {
        // create image
        vk::ImageCreateInfo imageInfo = vk::ImageCreateInfo()
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setImageType(vk::ImageType::e2D)
            .setFormat(format).setExtent(extent)
            .setMipLevels(1).setArrayLayers(1)
            .setUsage(usage);
        vma::AllocationCreateInfo allocInfo = vma::AllocationCreateInfo()
            .setUsage(vma::MemoryUsage::eAutoPreferDevice)
            .setRequiredFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
        std::tie(image, allocation) = alloc->createImageUnique(imageInfo, allocInfo);
        
        // create image view
        vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setImage(*image).setFormat(format)
            .setSubresourceRange(vk::ImageSubresourceRange(aspects, 
                0, vk::RemainingMipLevels, 
                0, vk::RemainingArrayLayers));
        view = device.createImageView(viewInfo);
    }

    using StageFlags = vk::PipelineStageFlags2;
    void transition_layout(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, 
            vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage,
            vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess) {
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(srcStage)
            .setSrcAccessMask(srcAccess)
            .setDstStageMask(dstStage)
            .setDstAccessMask(dstAccess)
            .setOldLayout(lastKnownLayout)
            .setNewLayout(layoutNew)
            .setSubresourceRange(vk::ImageSubresourceRange(aspects, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers))
            .setImage(*image);
        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);
        lastKnownLayout = layoutNew;
    }
    inline void transition_layout_r_to_w(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, StageFlags srcStage, StageFlags dstStage) {
        transition_layout(cmd, layoutNew, srcStage, dstStage, 
            vk::AccessFlagBits2::eMemoryRead, 
            vk::AccessFlagBits2::eMemoryWrite);
    }
    inline void transition_layout_w_to_r(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, StageFlags srcStage, StageFlags dstStage) {
        transition_layout(cmd, layoutNew, srcStage, dstStage, 
            vk::AccessFlagBits2::eMemoryWrite, 
            vk::AccessFlagBits2::eMemoryRead);
    }
    inline void transition_layout_r_to_rw(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, StageFlags srcStage, StageFlags dstStage) {
        transition_layout(cmd, layoutNew, srcStage, dstStage, 
            vk::AccessFlagBits2::eMemoryRead, 
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
    }
    inline void transition_layout_w_to_rw(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, StageFlags srcStage, StageFlags dstStage) {
        transition_layout(cmd, layoutNew, srcStage, dstStage, 
            vk::AccessFlagBits2::eMemoryWrite,
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
    }
    inline void transition_layout_rw_to_r(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, StageFlags srcStage, StageFlags dstStage) {
        transition_layout(cmd, layoutNew, srcStage, dstStage, 
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite, 
            vk::AccessFlagBits2::eMemoryRead);
    }
    inline void transition_layout_rw_to_w(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, StageFlags srcStage, StageFlags dstStage) {
        transition_layout(cmd, layoutNew, srcStage, dstStage, 
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite, 
            vk::AccessFlagBits2::eMemoryWrite);
    }
    inline void transition_layout_rw_to_rw(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew, StageFlags srcStage, StageFlags dstStage) {
        transition_layout(cmd, layoutNew, srcStage, dstStage, 
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite, 
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
    }

    vma::UniqueImage image;
    vma::UniqueAllocation allocation;
    vk::raii::ImageView view = nullptr;
    vk::Extent3D extent;
    vk::Format format;
    vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;
    vk::ImageLayout lastKnownLayout = vk::ImageLayout::eUndefined;
};
