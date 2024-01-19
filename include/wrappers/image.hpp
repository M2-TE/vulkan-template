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

    // TODO: replace the utils version with this
    void transition_layout_rw() {
        //lastKnownLayout = TODO
    }
    void transition_layout_wr() {

    }
    static void transition_layout_rw(Image& image) {

    }
    static void transition_layout_wr(Image& image) {

    }

    vma::UniqueImage image;
    vma::UniqueAllocation allocation;
    vk::raii::ImageView view = nullptr;
    vk::Extent3D extent;
    vk::Format format;
    vk::ImageLayout lastKnownLayout = vk::ImageLayout::eUndefined;
};
