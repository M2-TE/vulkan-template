#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <cmrc/cmrc.hpp>
#include <spirv_reflect.h>
#include <fmt/base.h>
//
#include "wrappers/image.hpp"

CMRC_DECLARE(shaders);

struct DescriptorAllocator {
    struct PoolSizeRatio {
        vk::DescriptorType type;
        float ratio;
    };
    void init_pool(vk::raii::Device& device, uint32_t maxSets, std::vector<PoolSizeRatio>& poolRatios) {
        std::vector<vk::DescriptorPoolSize> poolSizes;
        for (auto& ratio : poolRatios) {
            poolSizes.emplace_back(ratio.type, static_cast<uint32_t>(ratio.ratio * maxSets));
        }

        vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
            .setMaxSets(maxSets)
            .setPoolSizes(poolSizes);
        pool = device.createDescriptorPool(createInfo);
    }

    vk::raii::DescriptorPool pool = nullptr;
};

struct Shader {
	Shader(std::string path) : path(path.append(".spv")) {}
	void init(vk::raii::Device& device) {
		cmrc::embedded_filesystem fs = cmrc::shaders::get_filesystem();
		if (!fs.exists(path)) fmt::println("could not find shader: {}", path);
		cmrc::file file = fs.open(path);
		const uint32_t* pCode = reinterpret_cast<const uint32_t*>(file.cbegin());

		// load spir-v shader
		vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
			.setPCode(pCode)
			.setCodeSize(file.size());
		shader = device.createShaderModule(shaderInfo);

		// reflect spir-v shader contents
		spv_reflect::ShaderModule reflection(file.size(), pCode);
		stage = (vk::ShaderStageFlagBits)reflection.GetVulkanShaderStage();
		fmt::println("bindings: {}, sets: {}", reflection.GetShaderModule().descriptor_binding_count, reflection.GetShaderModule().descriptor_set_count);
	}

	void create_descriptors(vk::raii::Device& device, Image& image) {
        // todo: automate via reflection
        
        // create singular binding
        vk::DescriptorSetLayoutBinding descBinding = vk::DescriptorSetLayoutBinding()
            .setBinding(0)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eCompute)
            .setDescriptorType(vk::DescriptorType::eStorageImage);
        // create descriptor set layout from all bindings
        vk::DescriptorSetLayoutCreateInfo descLayoutInfo = vk::DescriptorSetLayoutCreateInfo()
            .setBindings(descBinding);
        descSetLayout = device.createDescriptorSetLayout(descLayoutInfo);

        // create descriptor pool
        std::vector<vk::DescriptorPoolSize> poolSizes = {
            vk::DescriptorPoolSize(descBinding.descriptorType, 1/*expand later*/)
        };
        vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo()
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
            .setMaxSets(1/*expand later*/)
            .setPoolSizes(poolSizes);
        pool = device.createDescriptorPool(poolCreateInfo);

        // allocate descriptor set from pool
        vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(*pool)
            .setSetLayouts(*descSetLayout);
        descSet = std::move(device.allocateDescriptorSets(allocInfo)[0]);

        //vk::Sampler samplerTODO;
        vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            //.setSampler(samplerTODO)
            .setImageView(*image.view);
        vk::WriteDescriptorSet drawImageWrite = vk::WriteDescriptorSet()
            .setDstBinding(0)
            .setDstSet(*descSet)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setImageInfo(imageInfo);
        device.updateDescriptorSets(drawImageWrite, {});
	}

	void print_info() {
		// figure out name and such
		std::string_view view(path);
		size_t delimPos = view.find_first_of('.', 0);
		std::string_view shaderName = view.substr(0, delimPos);
		std::string_view shaderType = view.substr(delimPos + 1, 4);
		fmt::println("name: {}, type: {}", shaderName, shaderType);
	}

    // raw shader
	std::string path;
	vk::raii::ShaderModule shader = nullptr; // doesnt need to persist after pipeline creation
	// descriptors
	vk::raii::DescriptorPool pool = nullptr;
	vk::raii::DescriptorSet descSet = nullptr;
	vk::raii::DescriptorSets descSets = nullptr;
	vk::raii::DescriptorSetLayout descSetLayout = nullptr;
    std::vector<vk::raii::DescriptorSetLayout> descSetLayouts;
	vk::ShaderStageFlags stage;
};