#include "Engine/UI/Menu.h"
#include <algorithm>

std::vector<UIElement*> Menu::ValidFocusables() const {
    std::vector<UIElement*> v;
    for (const auto& c : children) {
        if (c && c->visible && c->enabled && c->focusable) v.push_back(c.get());
    }
    return v;
}

void Menu::SetFocus(UIElement* element) {
    if (element == focus) return;
    if (focus) focus->SetFocused(false);
    focus = element;
    if (focus) focus->SetFocused(true);
}

void Menu::ClearFocus() {
    if (focus) focus->SetFocused(false);
    focus = nullptr;
}

void Menu::EnsureFocusValid() {
    std::vector<UIElement*> v = ValidFocusables();
    if (focus && std::find(v.begin(), v.end(), focus) != v.end()) return;   // 仍有效
    if (v.empty()) { ClearFocus(); return; }
    SetFocus(v.front());
}

void Menu::FocusNext() {
    std::vector<UIElement*> v = ValidFocusables();
    if (v.empty()) { ClearFocus(); return; }
    int idx = -1;
    for (int i = 0; i < static_cast<int>(v.size()); ++i) if (v[i] == focus) { idx = i; break; }
    int next = (idx + 1) % static_cast<int>(v.size());
    SetFocus(v[next]);
}

void Menu::FocusPrev() {
    std::vector<UIElement*> v = ValidFocusables();
    if (v.empty()) { ClearFocus(); return; }
    int idx = -1;
    for (int i = 0; i < static_cast<int>(v.size()); ++i) if (v[i] == focus) { idx = i; break; }
    int n = static_cast<int>(v.size());
    int prev = (idx <= 0) ? (n - 1) : (idx - 1);
    SetFocus(v[prev]);
}

void Menu::Open() {
    open = true;
    visible = true;
    EnsureFocusValid();
}

void Menu::Close() {
    open = false;
    visible = false;
    ClearFocus();
}

void Menu::Update(float dt) {
    UIContainer::Update(dt);   // children update + layout
    if (open) EnsureFocusValid();
}

void Menu::Render() {
    if (!open || !visible) return;
    UIContainer::Render();
}
