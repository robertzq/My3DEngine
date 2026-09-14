#include "Engine/Input.h"
#include "Engine/Log.h"
#include <cctype>
#include <map>
#include <utility>

namespace {

std::map<std::string, std::vector<SDL_Scancode>> actionKeys;
std::map<std::string, std::pair<std::string, std::string>> axes;

bool currentDown[SDL_NUM_SCANCODES] = {};
bool previousDown[SDL_NUM_SCANCODES] = {};
bool pressedThisFrame[SDL_NUM_SCANCODES] = {};
bool releasedThisFrame[SDL_NUM_SCANCODES] = {};

SDL_Scancode ParseKey(const std::string& name) {
    if (name.empty()) return SDL_SCANCODE_UNKNOWN;

    if (name.size() == 1) {
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
        if (c >= 'A' && c <= 'Z') return static_cast<SDL_Scancode>(SDL_SCANCODE_A + (c - 'A'));
        if (c >= '1' && c <= '9') return static_cast<SDL_Scancode>(SDL_SCANCODE_1 + (c - '1'));
        if (c == '0') return SDL_SCANCODE_0;
    }

    std::string upper;
    upper.reserve(name.size());
    for (char c : name) upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (upper == "LEFT") return SDL_SCANCODE_LEFT;
    if (upper == "RIGHT") return SDL_SCANCODE_RIGHT;
    if (upper == "UP") return SDL_SCANCODE_UP;
    if (upper == "DOWN") return SDL_SCANCODE_DOWN;
    if (upper == "SPACE") return SDL_SCANCODE_SPACE;
    if (upper == "RETURN" || upper == "ENTER") return SDL_SCANCODE_RETURN;
    if (upper == "ESCAPE" || upper == "ESC") return SDL_SCANCODE_ESCAPE;
    if (upper == "TAB") return SDL_SCANCODE_TAB;
    if (upper == "SHIFT") return SDL_SCANCODE_LSHIFT;
    if (upper == "CTRL" || upper == "CONTROL") return SDL_SCANCODE_LCTRL;
    if (upper == "ALT") return SDL_SCANCODE_LALT;
    if (upper == "BACKSPACE") return SDL_SCANCODE_BACKSPACE;

    return SDL_GetScancodeFromName(name.c_str());
}

bool AnyBound(const std::string& action, const bool (&table)[SDL_NUM_SCANCODES]) {
    auto it = actionKeys.find(action);
    if (it == actionKeys.end()) return false;
    for (SDL_Scancode sc : it->second) {
        if (table[sc]) return true;
    }
    return false;
}

}

namespace Input {

bool IsKeyDown(SDL_Scancode key) {
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    return state != nullptr && state[key] != 0;
}

void Bind(const std::string& action, const std::vector<std::string>& keys) {
    std::vector<SDL_Scancode> scancodes;
    scancodes.reserve(keys.size());
    for (const std::string& name : keys) {
        SDL_Scancode sc = ParseKey(name);
        if (sc == SDL_SCANCODE_UNKNOWN) {
            LOG_WARN("Input: 无法识别的按键名 -> " << name);
            continue;
        }
        scancodes.push_back(sc);
    }
    actionKeys[action] = std::move(scancodes);
}

void BindAxis(const std::string& axis, const std::string& negative, const std::string& positive) {
    axes[axis] = {negative, positive};
}

void Update() {
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    for (int sc = 0; sc < SDL_NUM_SCANCODES; ++sc) {
        bool down = state != nullptr && state[sc] != 0;
        currentDown[sc] = down;
        pressedThisFrame[sc] = down && !previousDown[sc];
        releasedThisFrame[sc] = !down && previousDown[sc];
        previousDown[sc] = down;
    }
}

bool Down(const std::string& action) { return AnyBound(action, currentDown); }
bool Pressed(const std::string& action) { return AnyBound(action, pressedThisFrame); }
bool Released(const std::string& action) { return AnyBound(action, releasedThisFrame); }

bool AnyPressed() {
    for (int sc = 0; sc < SDL_NUM_SCANCODES; ++sc) {
        if (pressedThisFrame[sc]) return true;
    }
    return false;
}

int Axis(const std::string& axis) {
    auto it = axes.find(axis);
    if (it == axes.end()) return 0;
    int value = 0;
    if (Down(it->second.first)) value -= 1;
    if (Down(it->second.second)) value += 1;
    return value;
}

}
