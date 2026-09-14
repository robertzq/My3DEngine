#include "Engine/UI/MenuStack.h"
#include "Engine/UI/Menu.h"
#include "Engine/UI/MenuInput.h"

void MenuStack::Push(std::unique_ptr<Menu> menu) {
    if (!menu) return;
    if (!stack.empty()) stack.back()->SetActive(false);   // 下层保留但不响应
    menu->Open();
    menu->SetActive(true);
    stack.push_back(std::move(menu));
}

bool MenuStack::Pop() {
    if (stack.empty()) return false;
    stack.pop_back();   // 销毁 top
    ActivateTop();
    return true;
}

void MenuStack::Replace(std::unique_ptr<Menu> menu) {
    if (!menu) return;
    if (!stack.empty()) stack.pop_back();
    if (!stack.empty()) stack.back()->SetActive(false);
    menu->Open();
    menu->SetActive(true);
    stack.push_back(std::move(menu));
}

void MenuStack::Clear() {
    stack.clear();
}

Menu* MenuStack::Top() const {
    return stack.empty() ? nullptr : stack.back().get();
}

void MenuStack::ActivateTop() {
    if (!stack.empty()) stack.back()->SetActive(true);
}

void MenuStack::HandleInput(float dt) {
    if (!stack.empty()) MenuInput::Handle(*stack.back(), dt);
}

void MenuStack::Update(float dt) {
    for (auto& m : stack) if (m) m->Update(dt);
}

void MenuStack::Render() {
    for (auto& m : stack) if (m) m->Render();
}
