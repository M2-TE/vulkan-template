#pragma once
#include <string_view>
//
#include "vk_wrappers/shader.hpp" // todo: hide the shader from outside?

namespace Pipelines {
	struct Compute {
		Compute(std::string_view path_cs): cs(std::string(path_cs).append(".spv")) {}
		void init(vk::raii::Device& device) {
			cs.init(device);
			vk::raii::ShaderModule csModule = cs.compile(device);

			std::vector<vk::DescriptorSetLayout> layouts;
			for (const auto& set : cs.descSetLayouts) layouts.emplace_back(*set);
			vk::PipelineLayoutCreateInfo layoutInfo = vk::PipelineLayoutCreateInfo()
				.setSetLayouts(layouts);
			layout = device.createPipelineLayout(layoutInfo);
			vk::PipelineShaderStageCreateInfo stageInfo = vk::PipelineShaderStageCreateInfo()
				.setModule(csModule)
				.setStage(vk::ShaderStageFlagBits::eCompute)
				.setPName("main");
			vk::ComputePipelineCreateInfo pipeInfo = vk::ComputePipelineCreateInfo()
				.setLayout(*layout)
				.setStage(stageInfo);
			pipeline = device.createComputePipeline(nullptr, pipeInfo);
		}
		void execute(vk::raii::CommandBuffer& cmd, uint32_t x, uint32_t y, uint32_t z) {
			cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, cs.descSets, {});
			cmd.dispatch(x, y, z);
		}

		Shader cs;
		vk::raii::Pipeline pipeline = nullptr;
		vk::raii::PipelineLayout layout = nullptr;
	};
	struct Graphics {
		Graphics(std::string_view path_vs, std::string_view path_fs): vs(std::string(path_vs).append(".spv")), fs(std::string(path_fs).append(".spv")) {}
		Shader vs, fs;
	};
}