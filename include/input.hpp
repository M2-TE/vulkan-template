#pragma once
#include <SDL3/SDL_events.h>
#include <imgui.h>
//
#include <string_view>
#include <cstdint>
#include <set>

namespace Input {
	struct Data {
		static Data& get() noexcept { static Data instance; return instance; }
		std::set<SDL_Keycode> keysPressed;
		std::set<SDL_Keycode> keysDown;
		std::set<SDL_Keycode> keysReleased;
		std::set<uint8_t> buttonsPressed;
		std::set<uint8_t> buttonsDown;
		std::set<uint8_t> buttonsReleased;
		float x, y;
		float dx, dy;
	};

	struct Keys {
        static inline bool pressed (std::string_view characters) noexcept {
            for (auto cur = characters.cbegin(); cur < characters.cend(); cur++) {
                if (pressed(*cur)) return true;
            }
            return false;
        }
		static inline bool pressed(char character) noexcept { return Data::get().keysPressed.contains(character) || Data::get().keysPressed.contains(character + 32); }
		static inline bool pressed(SDL_KeyCode code) noexcept { return Data::get().keysPressed.contains(code); }
        static inline bool down(std::string_view characters) noexcept {
            for (auto cur = characters.cbegin(); cur < characters.cend(); cur++) {
                if (down(*cur)) return true;
            }
            return false;
        }
		static inline bool down(char character) noexcept { return Data::get().keysDown.contains(character) || Data::get().keysPressed.contains(character + 32); }
		static inline bool down(SDL_KeyCode code) noexcept { return Data::get().keysDown.contains(code); }
        static inline bool released(std::string_view characters) noexcept {
            for (auto cur = characters.cbegin(); cur < characters.cend(); cur++) {
                if (released(*cur)) return true;
            }
            return false;
        }
		static inline bool released(char character) noexcept { return Data::get().keysReleased.contains(character) || Data::get().keysPressed.contains(character + 32); }
		static inline bool released(SDL_KeyCode code) noexcept { return Data::get().keysReleased.contains(code); }
    };
	struct Mouse {
		struct ids { static constexpr uint8_t left = SDL_BUTTON_LEFT, right = SDL_BUTTON_RIGHT, middle = SDL_BUTTON_MIDDLE; };
		static inline bool pressed(uint8_t buttonID) noexcept { return Data::get().buttonsPressed.contains(buttonID); }
		static inline bool down(uint8_t buttonID) noexcept { return Data::get().buttonsDown.contains(buttonID); }
		static inline bool released(uint8_t buttonID) noexcept { return Data::get().buttonsReleased.contains(buttonID); }
		static inline std::pair<decltype(Data::x), decltype(Data::y)> position() noexcept { return std::pair(Data::get().x, Data::get().y); };
		static inline std::pair<decltype(Data::dx), decltype(Data::dy)> delta() noexcept { return std::pair(Data::get().dx, Data::get().dy); };
	};
	
    static void flush() noexcept {
		Data::get().keysPressed.clear();
		Data::get().keysReleased.clear();
		Data::get().buttonsPressed.clear();
		Data::get().buttonsReleased.clear();
		Data::get().dx = 0;
		Data::get().dy = 0;
	}
	static void flush_all() noexcept {
		flush();
		Data::get().keysDown.clear();
		Data::get().buttonsDown.clear();
	}
	static void register_key_up(SDL_KeyboardEvent& keyEvent) noexcept {
		if (keyEvent.repeat || ImGui::GetIO().WantCaptureKeyboard) return;
		Data::get().keysReleased.insert(keyEvent.keysym.sym);
		Data::get().keysDown.erase(keyEvent.keysym.sym);
	}
	static void register_key_down(SDL_KeyboardEvent& keyEvent) noexcept {
		if (keyEvent.repeat || ImGui::GetIO().WantCaptureKeyboard) return;
		Data::get().keysPressed.insert(keyEvent.keysym.sym);
		Data::get().keysDown.insert(keyEvent.keysym.sym);
	}
	static void register_motion(SDL_MouseMotionEvent& motionEvent) noexcept {
		Data::get().dx += motionEvent.xrel;
		Data::get().dy += motionEvent.yrel;
		Data::get().x += motionEvent.xrel;
		Data::get().y += motionEvent.yrel;
	}
	static void register_button_up(SDL_MouseButtonEvent& buttonEvent) noexcept {
		if (ImGui::GetIO().WantCaptureMouse) return;
		Data::get().buttonsReleased.insert(buttonEvent.button);
		Data::get().buttonsDown.erase(buttonEvent.button);
	}
	static void register_button_down(SDL_MouseButtonEvent& buttonEvent) noexcept {
		if (ImGui::GetIO().WantCaptureMouse) return;
		Data::get().buttonsPressed.insert(buttonEvent.button);
		Data::get().buttonsDown.insert(buttonEvent.button);
	}
    static void register_event(SDL_Event& event) noexcept {
        switch(event.type) {
            case SDL_EventType::SDL_EVENT_KEY_UP: register_key_up(event.key); break;
            case SDL_EventType::SDL_EVENT_KEY_DOWN: register_key_down(event.key); break;
            case SDL_EventType::SDL_EVENT_MOUSE_MOTION: register_motion(event.motion); break;
            case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP: register_button_up(event.button); break;
            case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_DOWN: register_button_down(event.button); break;
        }
    }
}
typedef Input::Keys Keys;
typedef Input::Mouse Mouse;