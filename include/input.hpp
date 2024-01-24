#pragma once
#include <SDL3/SDL_events.h>
#include <imgui.h>
//
#include <cstdint>
#include <utility>
#include <set>

namespace input {
	static std::set<SDL_Keycode> keysPressed;
	static std::set<SDL_Keycode> keysDown;
	static std::set<SDL_Keycode> keysReleased;
	static std::set<uint8_t> buttonsPressed;
	static std::set<uint8_t> buttonsDown;
	static std::set<uint8_t> buttonsReleased;

	namespace Keys {
		static inline bool pressed(SDL_KeyCode code) { return keysPressed.contains(code); }
		static inline bool pressed(char character) { return keysPressed.contains((SDL_KeyCode)character); }
		static inline bool down(SDL_KeyCode code) { return keysDown.contains(code); }
		static inline bool down(char character) { return keysDown.contains((SDL_KeyCode)character); }
		static inline bool released(SDL_KeyCode code) { return keysReleased.contains(code); }
		static inline bool released(char character) { return keysReleased.contains((SDL_KeyCode)character); }
	}
	namespace Mouse {
		namespace ids {
			static constexpr uint8_t left = 1;
			static constexpr uint8_t right = 3;
			static constexpr uint8_t middle = 2;
		}
		static float x, y;
		static float dx, dy;

		static inline bool pressed(uint8_t buttonID) { return buttonsPressed.contains(buttonID); }
		static inline bool down(uint8_t buttonID) { return buttonsDown.contains(buttonID); }
		static inline bool released(uint8_t buttonID) { return buttonsReleased.contains(buttonID); }
	}
	static void flush() {
		keysPressed.clear();
		keysReleased.clear();
		buttonsPressed.clear();
		buttonsReleased.clear();
		Mouse::dx = 0;
		Mouse::dy = 0;
	}
	static void flush_all() {
		flush();
		keysDown.clear();
		buttonsDown.clear();
	}

	static void register_key_up(SDL_KeyboardEvent& keyEvent) {
		if (keyEvent.repeat || ImGui::GetIO().WantCaptureKeyboard) return;
		keysReleased.insert(keyEvent.keysym.sym);
		keysDown.erase(keyEvent.keysym.sym);
	}
	static void register_key_down(SDL_KeyboardEvent& keyEvent) {
		if (keyEvent.repeat || ImGui::GetIO().WantCaptureKeyboard) return;
		keysPressed.insert(keyEvent.keysym.sym);
		keysDown.insert(keyEvent.keysym.sym);
	}
	static void register_motion(SDL_MouseMotionEvent& motionEvent) {
		Mouse::dx += motionEvent.xrel;
		Mouse::dy += motionEvent.yrel;
		Mouse::x += motionEvent.xrel;
		Mouse::y += motionEvent.yrel;
	}
	static void register_button_up(SDL_MouseButtonEvent& buttonEvent) {
		if (ImGui::GetIO().WantCaptureMouse) return;
		buttonsReleased.insert(buttonEvent.button);
		buttonsDown.erase(buttonEvent.button);
	}
	static void register_button_down(SDL_MouseButtonEvent& buttonEvent) {
		if (ImGui::GetIO().WantCaptureMouse) return;
		buttonsPressed.insert(buttonEvent.button);
		buttonsDown.insert(buttonEvent.button);
	}
}
namespace Keys = input::Keys;
namespace Mouse = input::Mouse;