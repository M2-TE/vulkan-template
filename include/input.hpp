#pragma once
#include <SDL3/SDL_events.h>
#include <imgui.h>
//
#include <cstdint>
#include <utility>
#include <set>

namespace input {
	namespace key {
		static std::set<SDL_Keycode> keysPressed;
		static std::set<SDL_Keycode> keysDown;
		static std::set<SDL_Keycode> keysReleased;

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

		static inline bool pressed(SDL_KeyCode code) { return keysPressed.contains(code); }
		static inline bool pressed(char character) { return keysPressed.contains((SDL_KeyCode)character); }
		static inline bool down(SDL_KeyCode code) { return keysDown.contains(code); }
		static inline bool down(char character) { return keysDown.contains((SDL_KeyCode)character); }
		static inline bool released(SDL_KeyCode code) { return keysReleased.contains(code); }
		static inline bool released(char character) { return keysReleased.contains((SDL_KeyCode)character); }
	}
	namespace mouse {
		static constexpr uint8_t left = 1;
		static constexpr uint8_t right = 3;
		static constexpr uint8_t middle = 2;
		static float x, y;
		static float dx, dy;
		static std::set<uint8_t> buttonsPressed;
		static std::set<uint8_t> buttonsDown;
		static std::set<uint8_t> buttonsReleased;

		static void register_motion(SDL_MouseMotionEvent& motionEvent) {
			dx += motionEvent.xrel;
			dy += motionEvent.yrel;
			x += dx;
			y += dy;
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

		static inline bool pressed(uint8_t buttonID) { return buttonsPressed.contains(buttonID); }
		static inline bool down(uint8_t buttonID) { return buttonsDown.contains(buttonID); }
		static inline bool released(uint8_t buttonID) { return buttonsReleased.contains(buttonID); }
	}
	static void flush() {
		mouse::dx = 0;
		mouse::dy = 0;
		key::keysPressed.clear();
		key::keysReleased.clear();
		mouse::buttonsPressed.clear();
		mouse::buttonsReleased.clear();
	}
	static void flush_all() {
		flush();
		key::keysDown.clear();
		mouse::buttonsDown.clear();
	}

}
namespace Keys = input::key;
namespace Mouse = input::mouse;