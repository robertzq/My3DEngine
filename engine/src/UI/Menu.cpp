#include "Engine/UI/Menu.h"
#include "Engine/UI/UIPrimitives.h"
#include <algorithm>
#include <utility>

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

void Menu::SetActive(bool a) {
    active = a;
    if (focus) focus->SetFocused(a);
    if (a) EnsureFocusValid();
}

void Menu::Close() {
    open = false;
    visible = false;
    ClearFocus();
}

void Menu::Update(float dt) {
    UIContainer::Update(dt);   // children update + layout
    if (open && active) EnsureFocusValid();
}

void Menu::Render() {
    if (!open || !visible) return;
    UIContainer::Render();
}

void Menu::EmitAction(const std::string& elementId, const std::string& action, nlohmann::json value) {
    actions.push_back(UIAction{id, elementId, action, std::move(value)});
}

std::vector<UIAction> Menu::ConsumeActions() {
    std::vector<UIAction> out = std::move(actions);
    actions.clear();
    return out;
}

void Menu::Confirm() { EmitAction(std::string(), "confirm", nlohmann::json()); }
void Menu::Cancel()  { EmitAction(std::string(), "cancel", nlohmann::json()); }

void Menu::ActivateFocused() {
    UIElement* f = focus;
    if (!f) { Confirm(); return; }

    if (Button* b = dynamic_cast<Button*>(f)) {
        b->Activate();
        EmitAction(b->id, b->action.empty() ? std::string("activate") : b->action, {});
    } else if (Toggle* t = dynamic_cast<Toggle*>(f)) {
        t->Activate();
        EmitAction(t->id, "value_changed", t->value);
    } else if (Slider* s = dynamic_cast<Slider*>(f)) {
        (void)s;   // confirm 不改滑条值；用 left/right
    } else {
        f->Activate();
    }
}

void Menu::AdjustFocused(int dir) {
    UIElement* f = focus;
    if (!f) return;
    f->Adjust(dir);

    if (Toggle* t = dynamic_cast<Toggle*>(f)) EmitAction(t->id, "value_changed", t->value);
    else if (Slider* s = dynamic_cast<Slider*>(f)) EmitAction(s->id, "value_changed", s->value);
}
