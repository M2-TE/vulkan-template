#include <spirv_reflect.h>
#include <cmrc/cmrc.hpp>
#include <fmt/base.h>
#undef VULKAN_HPP_NO_TO_STRING
#include <vulkan/vulkan_raii.hpp>
//
#include <unordered_map>
#include <span>
//
#include "vk_wrappers/shader.hpp"
CMRC_DECLARE(shaders);

static inline std::pair<const uint32_t*, size_t> read_data(std::string& path) {
    cmrc::embedded_filesystem fs = cmrc::shaders::get_filesystem();
    if (!fs.exists(path)) {
        fmt::println("could not find shader: {}", path);
        exit(-1);
    }
    cmrc::file file = fs.open(path);
    return std::pair(reinterpret_cast<const uint32_t*>(file.cbegin()), file.size());
}
static inline std::vector<SpvReflectDescriptorSet*> enumDescSets(const spv_reflect::ShaderModule& reflection) {
    uint32_t nDescSets;
    SpvReflectResult result;
    std::vector<SpvReflectDescriptorSet*> reflDescSets;
    result = reflection.EnumerateDescriptorSets(&nDescSets, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) fmt::println("shader reflection error: {}", (uint32_t)result);
    reflDescSets.resize(nDescSets);
    result = reflection.EnumerateDescriptorSets(&nDescSets, reflDescSets.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) fmt::println("shader reflection error: {}", (uint32_t)result);
    return reflDescSets;
}
static inline std::vector<SpvReflectDescriptorBinding*> enumDescBindings(const spv_reflect::ShaderModule& reflection) {
    uint32_t nDescBinds;
    SpvReflectResult result;
    std::vector<SpvReflectDescriptorBinding*> reflDescBinds;
    result = reflection.EnumerateDescriptorBindings(&nDescBinds, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) fmt::println("shader reflection error: {}", (uint32_t)result);
    reflDescBinds.resize(nDescBinds);
    result = reflection.EnumerateDescriptorBindings(&nDescBinds, reflDescBinds.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) fmt::println("shader reflection error: {}", (uint32_t)result);
    return reflDescBinds;
}
static inline std::vector<vk::DescriptorPoolSize> getPoolSizes(std::span<SpvReflectDescriptorBinding*> reflDescBinds) {
    std::unordered_map<vk::DescriptorType, uint32_t> bindingLookup;
    // tally count of all bind types
    for (const auto& descBind : reflDescBinds) {
        auto [resa, resb] = bindingLookup.try_emplace((vk::DescriptorType)descBind->descriptor_type, 1);
        if (!resb) resa->second++;
    }
    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(bindingLookup.size());
    for (const auto& pair : bindingLookup) poolSizes.emplace_back(pair.first, pair.second);
    return poolSizes;
}

Shader::Shader(std::string_view path): path(path) {}
void Shader::init(vk::raii::Device& device) {
    // reflect spir-v shader contents
    auto [pData, size] = read_data(path);
    const spv_reflect::ShaderModule reflection(size, pData);

    // stage, sets, bindings
    stage = (vk::ShaderStageFlags)reflection.GetShaderStage();
    std::vector<SpvReflectDescriptorSet*> reflDescSets = enumDescSets(reflection);
    std::vector<SpvReflectDescriptorBinding*> reflDescBinds = enumDescBindings(reflection);
    if (reflDescSets.size() == 0) return;
    fmt::println("{}: {} set(s) and {} binding(s)", path, reflDescSets.size(), reflDescBinds.size());

    // create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes = getPoolSizes(reflDescBinds);
    vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo({}, reflDescSets.size(), poolSizes);
    pool = device.createDescriptorPool(poolCreateInfo);

    // enumerate all sets
    descSetLayouts.reserve(reflDescSets.size());
    for (const auto& set : reflDescSets) {
        // enumerate all bindings for current set
        std::vector<vk::DescriptorSetLayoutBinding> bindings(set->binding_count);
        for (uint32_t i = 0; i < set->binding_count; i++) {
            SpvReflectDescriptorBinding* pBinding = set->bindings[i];
            fmt::println("\tset {} | binding {}: {} {}", 
                    pBinding->set, 
                    pBinding->binding,
                    vk::to_string((vk::DescriptorType)pBinding->descriptor_type), 
                    pBinding->name);
            bindings[i].setBinding(pBinding->binding)
                .setDescriptorCount(pBinding->count)
                .setStageFlags((vk::ShaderStageFlagBits)reflection.GetShaderStage())
                .setDescriptorType((vk::DescriptorType)pBinding->descriptor_type);
        }

        // create set layout from all bindings
        vk::DescriptorSetLayoutCreateInfo descLayoutInfo = vk::DescriptorSetLayoutCreateInfo({}, bindings);
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
    auto [pData, size] = read_data(path);
    vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
        .setPCode(pData)
        .setCodeSize(size);
    return device.createShaderModule(shaderInfo);
}