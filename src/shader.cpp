#include <spirv_reflect.h>
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(shaders);
#include <fmt/base.h>
//
#include "vk_wrappers/shader.hpp"

Shader::Shader(std::string_view path): path(path) {}
void Shader::init(vk::raii::Device& device) {
    cmrc::embedded_filesystem fs = cmrc::shaders::get_filesystem();
    if (!fs.exists(path)) {
        fmt::println("could not find shader: {}", path);
        exit(-1);
    }
    cmrc::file file = fs.open(path);
    const uint32_t* pCode = reinterpret_cast<const uint32_t*>(file.cbegin());

    // load spir-v shader
    vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
        .setPCode(pCode)
        .setCodeSize(file.size());
    shader = compile(device);

    // reflect spir-v shader contents
    const spv_reflect::ShaderModule reflection(file.size(), pCode);
    stage = (vk::ShaderStageFlagBits)reflection.GetShaderStage();

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
                .setStageFlags((vk::ShaderStageFlagBits)reflection.GetShaderStage())
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

vk::raii::ShaderModule Shader::compile(vk::raii::Device& device) {
    cmrc::embedded_filesystem fs = cmrc::shaders::get_filesystem();
    if (!fs.exists(path)) {
        fmt::println("could not find shader: {}", path);
        exit(-1);
    }
    cmrc::file file = fs.open(path);
    const uint32_t* pCode = reinterpret_cast<const uint32_t*>(file.cbegin());

    // load spir-v shader
    vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
        .setPCode(pCode)
        .setCodeSize(file.size());
    return device.createShaderModule(shaderInfo);
}