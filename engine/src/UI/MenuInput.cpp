#include "Engine/UI/MenuInput.h"
#include "Engine/UI/Menu.h"
#include "Engine/Input.h"

namespace {

// Slider 长按连发：首次立即调整，暂停 RepeatDelay 后每 RepeatInterval 再调整。
UIElement* repeatTarget = nullptr;
int repeatDir = 0;
float repeatTimer = 0.0f;
constexpr float RepeatDelay = 0.35f;
constexpr float RepeatInterval = 0.06f;

void ResetRepeat() {
    repeatTarget = nullptr;
    repeatDir = 0;
    repeatTimer = 0.0f;
}

}

namespace MenuInput {

void Handle(Menu& menu, float dt) {
    if (!menu.IsOpen()) { ResetRepeat(); return; }

    if (Input::Pressed("UIUp")) { menu.FocusPrev(); ResetRepeat(); }
    if (Input::Pressed("UIDown")) { menu.FocusNext(); ResetRepeat(); }

    UIElement* focused = menu.Focused();

    if (Input::Pressed("UIConfirm")) menu.ActivateFocused();
    if (Input::Pressed("UICancel")) menu.Cancel();

    // 左右：可连发组件（Slider）按住连发；其余（Toggle）只在按下的边沿调整一次。
    bool leftDown = Input::Down("UILeft");
    bool rightDown = Input::Down("UIRight");
    int dir = (leftDown == rightDown) ? 0 : (leftDown ? -1 : +1);

    if (!focused || dir == 0 || !focused->RepeatableAdjust()) {
        ResetRepeat();
        if (focused) {
            if (Input::Pressed("UILeft")) menu.AdjustFocused(-1);
            if (Input::Pressed("UIRight")) menu.AdjustFocused(+1);
        }
        return;
    }

    if (focused != repeatTarget || dir != repeatDir) {
        repeatTarget = focused;
        repeatDir = dir;
        repeatTimer = 0.0f;
        menu.AdjustFocused(dir);        // 首次立即响应
        return;
    }

    repeatTimer += dt;
    while (repeatTimer >= RepeatDelay) {
        repeatTimer -= RepeatInterval;
        menu.AdjustFocused(dir);
        if (RepeatInterval <= 0.0f) break;
    }
}

}
