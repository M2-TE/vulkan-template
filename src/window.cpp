#include <vulkan/vulkan_raii.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <fmt/base.h>
//
#include <vector>
//
#include "window.hpp"
#include "wrappers/queues.hpp"
#include "wrappers/swapchain.hpp"

#ifdef VULKAN_DEBUG_MSG_UTILS
#define MSG_UTILS(x) x
#define MSG_UTILS_REQUESTED 1
#else
#define MSG_UTILS(x)
#define MSG_UTILS_REQUESTED 0
#endif

static void imgui_clear(); // todo: remove forward decl
Window::Window() {
    // SDL: init subsystem
    if (SDL_InitSubSystem(SDL_InitFlags::SDL_INIT_VIDEO)) fmt::println("{}", SDL_GetError());

    // SDL: create window
    pWindow = SDL_CreateWindow(name.c_str(), extent.width, extent.height, SDL_WINDOW_VULKAN);
    if (pWindow == nullptr) fmt::println("{}", SDL_GetError());
    
    // SDL: query required extensions
    uint32_t nExtensions;
    const char* const* pExtensions = SDL_Vulkan_GetInstanceExtensions(&nExtensions);
    if (pExtensions == nullptr) fmt::println("{}", SDL_GetError());
    extensions.resize(nExtensions);
    for (uint32_t i = 0; i < nExtensions; i++) extensions[i] = pExtensions[i];

    // ImGui: do stuff
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(pWindow);
}
Window::~Window() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    imgui_clear();
    SDL_Quit();
}
void Window::init(vk::raii::Instance& instance, vk::DebugUtilsMessengerEXT msg) {
    // SDL: create surface
    VkSurfaceKHR surfaceTemp;
    if (!SDL_Vulkan_CreateSurface(pWindow, *instance, nullptr, &surfaceTemp)) fmt::println("{}", SDL_GetError());
    surface = vk::raii::SurfaceKHR(instance, surfaceTemp);
    MSG_UTILS(debugMsg = vk::raii::DebugUtilsMessengerEXT(instance, msg));
}
bool Window::using_debug_msg() {
    return (bool)MSG_UTILS_REQUESTED;
}

// todo: shove these into their own static namespace
static vk::raii::DescriptorPool imguiDescPool = nullptr;
static void imgui_clear() { // todo: rename
    imguiDescPool.clear();
}
PFN_vkVoidFunction imgui_load_fnc(const char* function_name, void* user_data) {
    const vk::raii::Instance& instance = *reinterpret_cast<vk::raii::Instance*>(user_data);
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(*instance, function_name);
}
void Window::imgui_init(vk::raii::Instance& instance, vk::raii::Device& device, vk::raii::PhysicalDevice& physDevice, Queues& queues, vk::Format swapchainFormat) {
    bool success = ImGui_ImplVulkan_LoadFunctions(&imgui_load_fnc, &instance);
    if (!success) fmt::println("sdl failed to load vulkan functions");

    // create temporary descriptor pool for sdl
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)
    };
    vk::DescriptorPoolCreateInfo poolInfo = vk::DescriptorPoolCreateInfo()
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        .setMaxSets(1000)
        .setPoolSizes(poolSizes);
    imguiDescPool = device.createDescriptorPool(poolInfo);

    // initialize vulkan backend
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = *instance;
    initInfo.PhysicalDevice = *physDevice;
    initInfo.Device = *device;
    initInfo.Queue = *queues.graphics.queue;
    initInfo.DescriptorPool = *imguiDescPool;
    initInfo.ImageCount = 3;
    initInfo.MinImageCount = initInfo.ImageCount;
    initInfo.UseDynamicRendering = true;
    initInfo.ColorAttachmentFormat = (VkFormat)swapchainFormat;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo, nullptr);;
    ImGui_ImplVulkan_CreateFontsTexture();
}
bool Window::imgui_process_event(SDL_Event* pEvent) {
    return ImGui_ImplSDL3_ProcessEvent(pEvent);
}
void Window::imgui_new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}
void Window::imgui_draw(vk::raii::CommandBuffer& cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
}
