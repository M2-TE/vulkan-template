#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.hpp>
#include <SDL3/SDL_events.h>
#include <fmt/base.h>
#include <imgui.h>
//
#include <chrono>
#include <thread>
//
#include "input.hpp"
#include "window.hpp"
#include "renderer.hpp"
#include "imgui_impl.hpp"
#include "vk_wrappers/swapchain.hpp"
#include "vk_wrappers/queues.hpp"

struct Engine {
    Engine();
    void run() {
        bRunning = true;
        bRendering = true;
        nFrames = 0;
        while(bRunning) {
            Input::flush();
            SDL_Event event;
            while (SDL_PollEvent(&event)) handle_event(event);
            handle_input();

            if (bRendering) {
                ImGui::backend::new_frame();
                ImGui::frontend::display_fps();
                renderer.render(device, swapchain, queues);
                if (swapchain.bResizeRequested) handle_rebuild();
                nFrames++;
            }
            else std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        device.waitIdle();
        ImGui::backend::shutdown();
    }

private:
    void handle_event(SDL_Event& event) {
        ImGui::backend::process_event(&event);
        switch (event.type) {
            // window handling
            case SDL_EventType::SDL_EVENT_QUIT: bRunning = false; break;
            case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED: bRendering = false; break;
            case SDL_EventType::SDL_EVENT_WINDOW_RESTORED: bRendering = true; break;
            case SDL_EventType::SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: handle_rebuild(); break;
            // input handling
            case SDL_EventType::SDL_EVENT_KEY_UP: Input::register_key_up(event.key); break;
            case SDL_EventType::SDL_EVENT_KEY_DOWN: Input::register_key_down(event.key); break;
            case SDL_EventType::SDL_EVENT_MOUSE_MOTION: Input::register_motion(event.motion); break;
            case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP: Input::register_button_up(event.button); break;
            case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_DOWN: Input::register_button_down(event.button); break;
            case SDL_EventType::SDL_EVENT_WINDOW_FOCUS_LOST: Input::flush_all();
            default: break;
        }
    }
    void handle_rebuild() {
        SDL_SyncWindow(window.pWindow);
        device.waitIdle();
        if (window.size() != swapchain.extent) {
            renderer = {};
            renderer.init(device, alloc, queues, window.size());
            swapchain = {};
            swapchain.init(physDevice, device, window, queues);
        }
    }
    void handle_input() {
        if (Keys::pressed(SDLK_F11)) window.toggle_fullscreen();
    }

private:
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::PhysicalDevice physDevice = nullptr;
    vk::raii::Device device = nullptr;
    vma::UniqueAllocator alloc;
    Window window = { 1280, 720 };
    Swapchain swapchain;
    Queues queues;
    Renderer renderer;

    bool bRunning;
    bool bRendering;
    size_t nFrames;
};