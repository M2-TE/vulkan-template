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

    void transition_layout_rw(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew,
            vk::PipelineStageFlagBits2 srcStage, vk::PipelineStageFlagBits2 dstStage) {
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(srcStage)
            .setSrcAccessMask(vk::AccessFlagBits2::eMemoryRead)
            .setDstStageMask(dstStage)
            .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite)
            .setOldLayout(lastKnownLayout)
            .setNewLayout(layoutNew)
            .setSubresourceRange(vk::ImageSubresourceRange(aspects, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers))
            .setImage(*image);
        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(1)
            .setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);
        lastKnownLayout = layoutNew;
    }
    void transition_layout_wr(vk::raii::CommandBuffer& cmd, vk::ImageLayout layoutNew,
        vk::PipelineStageFlagBits2 srcStage, vk::PipelineStageFlagBits2 dstStage) {
        vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(srcStage)
            .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
            .setDstStageMask(dstStage)
            .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead)
            .setOldLayout(lastKnownLayout)
            .setNewLayout(layoutNew)
            .setSubresourceRange(vk::ImageSubresourceRange(aspects, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers))
            .setImage(*image);
        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(1)
            .setImageMemoryBarriers(imageBarrier);
        cmd.pipelineBarrier2(depInfo);
        lastKnownLayout = layoutNew;
    }

    vma::UniqueImage image;
    vma::UniqueAllocation allocation;
    vk::raii::ImageView view = nullptr;
    vk::Extent3D extent;
    vk::Format format;
    vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;
    vk::ImageLayout lastKnownLayout = vk::ImageLayout::eUndefined;
};
