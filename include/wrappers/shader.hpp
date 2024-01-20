#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <cmrc/cmrc.hpp>
#include <spirv_reflect.h>
#include <fmt/base.h>
//
#include <unordered_map>
//
#include "wrappers/image.hpp"

CMRC_DECLARE(shaders);

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
		const spv_reflect::ShaderModule reflection(file.size(), pCode);

        // read shader stage
		stage = (vk::ShaderStageFlagBits)reflection.GetVulkanShaderStage();
        // enumerate sets
        uint32_t nDescSets = 0;
        std::vector<SpvReflectDescriptorSet*> reflDescSets;
        assert(SPV_REFLECT_RESULT_SUCCESS == reflection.EnumerateDescriptorSets(&nDescSets, nullptr));
        reflDescSets.resize(nDescSets);
        assert(SPV_REFLECT_RESULT_SUCCESS == reflection.EnumerateDescriptorSets(&nDescSets, reflDescSets.data()));
        // enumerate bindings
        uint32_t nDescBinds = 0;
        std::vector<SpvReflectDescriptorBinding*> descBinds;
        assert(SPV_REFLECT_RESULT_SUCCESS == reflection.EnumerateDescriptorBindings(&nDescBinds, nullptr));
        descBinds.resize(nDescBinds);
        assert(SPV_REFLECT_RESULT_SUCCESS == reflection.EnumerateDescriptorBindings(&nDescBinds, descBinds.data()));
		fmt::println("sets: {}, bindings: {}", nDescSets, nDescBinds);

        // query which descriptor types will be allocated
        if (nDescSets == 0) return;
        std::vector<vk::DescriptorPoolSize> poolSizes;
        std::unordered_map<vk::DescriptorType, uint32_t> bindingLookup;
        // tally count of all bind types
        for (const auto& descBind : descBinds) {
            auto [resa, resb] = bindingLookup.try_emplace((vk::DescriptorType)descBind->descriptor_type, 1);
            if (!resb) resa->second++;
        }
        poolSizes.reserve(bindingLookup.size());
        for (const auto& pair : bindingLookup) poolSizes.emplace_back(pair.first, pair.second);
        // create pool
        vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo()
            .setMaxSets(nDescSets)
            .setPoolSizes(poolSizes);
        pool = device.createDescriptorPool(poolCreateInfo);

        // enumerate all sets
        descSetLayouts.reserve(nDescSets);
        for (const auto& set : reflDescSets) {

            // enumerate all bindings for current set
            std::vector<vk::DescriptorSetLayoutBinding> bindings(set->binding_count);
            for (uint32_t i = 0; i < set->binding_count; i++) {
                SpvReflectDescriptorBinding* pBinding = set->bindings[i];
                fmt::println("{} at binding: {}", pBinding->name, pBinding->binding);
                bindings[i].setBinding(pBinding->binding)
                    .setDescriptorCount(pBinding->count)
                    .setStageFlags((vk::ShaderStageFlagBits)reflection.GetVulkanShaderStage())
                    .setDescriptorType((vk::DescriptorType)pBinding->descriptor_type);
            }

            // create set layout from all bindings
            vk::DescriptorSetLayoutCreateInfo descLayoutInfo = vk::DescriptorSetLayoutCreateInfo()
                .setBindings(bindings);
            descSetLayouts.emplace_back(device.createDescriptorSetLayout(descLayoutInfo));
        }

        // allocate desc sets
        std::vector<vk::DescriptorSetLayout> layouts;
        for (const auto& set : descSetLayouts) layouts.emplace_back(*set);
        vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(*pool)
            .setSetLayouts(layouts);
        descSets = (*device).allocateDescriptorSets(allocInfo);
	}

    // todo: different layout/type based on shader stage? or extra param
	void write_descriptor(Image& image, uint32_t set, uint32_t binding) {
        vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(*image.view);
        vk::WriteDescriptorSet drawImageWrite = vk::WriteDescriptorSet()
            .setDstBinding(0)
            .setDstSet(descSets[0])
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setImageInfo(imageInfo);
        pool.getDevice().updateDescriptorSets(drawImageWrite, {});
	}

	std::string path;
	vk::ShaderStageFlags stage;
	vk::raii::ShaderModule shader = nullptr; // doesnt need to persist
	vk::raii::DescriptorPool pool = nullptr;
	std::vector<vk::DescriptorSet> descSets;
    std::vector<vk::raii::DescriptorSetLayout> descSetLayouts;

    // todo if desired:
    std::unordered_map<std::string, std::pair<uint32_t/*set*/, uint32_t/*binding*/>> namedDescriptors;
};