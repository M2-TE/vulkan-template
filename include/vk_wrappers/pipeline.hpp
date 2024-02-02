#pragma once
#include <string_view>
//
#include "vk_wrappers/shader.hpp" // todo: hide the shader from outside?

namespace Pipelines {
	struct Pipeline {
		// todo?
	};
	struct Compute: private Pipeline {
		Compute(std::string_view path_cs): cs(std::string(path_cs).append(".spv")) {

		}
		void init(vk::raii::Device& device) {
			// vk::raii::ShaderModule csModule = cs.compile(device);
			cs.init(device);

			std::vector<vk::DescriptorSetLayout> layouts;
			for (const auto& set : cs.descSetLayouts) layouts.emplace_back(*set);
			vk::PipelineLayoutCreateInfo layoutInfo = vk::PipelineLayoutCreateInfo()
				.setSetLayouts(layouts);
			layout = device.createPipelineLayout(layoutInfo);
			vk::PipelineShaderStageCreateInfo stageInfo = vk::PipelineShaderStageCreateInfo()
				.setModule(*cs.shader) // todo
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
	struct Graphics: private Pipeline {
		Graphics(std::string_view path_vs, std::string_view path_ps): vs(path_vs), ps(path_ps) {

		}

		Shader vs, ps;
	};
}