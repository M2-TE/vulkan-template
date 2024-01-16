#include <vulkan/vulkan_raii.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>
#include "window.hpp"

#ifdef VULKAN_DEBUG_MSG_UTILS
#define MSG_UTILS(x) x
#define MSG_UTILS_REQUESTED 1
#else
#define MSG_UTILS(x)
#define MSG_UTILS_REQUESTED 0
#endif

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
}
Window::~Window() {
    SDL_Quit();
}
void Window::init(vk::raii::Instance& instance, vk::DebugUtilsMessengerEXT msg) {
    // SDL: create surface
    VkSurfaceKHR surfaceTemp;
    if (!SDL_Vulkan_CreateSurface(pWindow, *instance, nullptr, &surfaceTemp)) fmt::println("{}", SDL_GetError());
    surface = vk::raii::SurfaceKHR(instance, surfaceTemp);
    MSG_UTILS(debugMsg = vk::raii::DebugUtilsMessengerEXT(instance, msg));
}
bool Window::use_debug_msg() {
    return (bool)MSG_UTILS_REQUESTED;
}