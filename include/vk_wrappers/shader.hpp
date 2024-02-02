#pragma once
#include <vulkan/vulkan_raii.hpp>
//
#include <string_view>
// #include <unordered_map>
//
#include "vk_wrappers/image.hpp"

struct Shader {
	Shader(std::string_view path);
    void init(vk::raii::Device& device); // todo: device needed?
    vk::raii::ShaderModule compile(vk::raii::Device& device);

    // todo: different layout/type based on shader stage
	void write_descriptor(Image& image, uint32_t set, uint32_t binding) {
        vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(*image.view);
        vk::WriteDescriptorSet drawImageWrite = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDstSet(descSets[set])
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setImageInfo(imageInfo);
        pool.getDevice().updateDescriptorSets(drawImageWrite, {});
	}

	std::string path;
	vk::ShaderStageFlags stage;
	vk::raii::ShaderModule shader = nullptr; // REMOVE
	vk::raii::DescriptorPool pool = nullptr;
	std::vector<vk::DescriptorSet> descSets;
    std::vector<vk::raii::DescriptorSetLayout> descSetLayouts;
};