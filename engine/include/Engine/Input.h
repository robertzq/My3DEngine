#pragma once
#include <SDL.h>

// 输入抽象层：使用键盘状态轮询，避免 KEYDOWN/KEYUP 造成的按键状态残留
// （例如窗口失焦时 KEYUP 事件丢失会导致角色持续移动）
class Input {
public:
    // 查询某个按键当前是否被按住（使用 scancode，与物理键位一致，不受输入法影响）
    static bool IsKeyDown(SDL_Scancode key) {
        const Uint8* state = SDL_GetKeyboardState(nullptr);
        return state != nullptr && state[key] != 0;
    }
};
