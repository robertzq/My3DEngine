#include "Engine/UI/UIContainer.h"

UIElement* UIContainer::Add(std::unique_ptr<UIElement> child) {
    UIElement* raw = child.get();
    children.push_back(std::move(child));
    return raw;
}

UIElement* UIContainer::Find(const std::string& id) const {
    for (const auto& c : children) {
        if (c && c->id == id) return c.get();
    }
    return nullptr;
}

void UIContainer::ApplyLayout() {
    if (!layout) return;
    std::vector<UIElement*> ptrs;
    ptrs.reserve(children.size());
    for (auto& c : children) ptrs.push_back(c.get());
    layout->Apply(ScreenRect(), ptrs);
}

void UIContainer::Update(float dt) {
    for (auto& c : children) {
        if (c) c->Update(dt);
    }
    ApplyLayout();
}

void UIContainer::Render() {
    if (!visible) return;
    for (auto& c : children) {
        if (c && c->visible) c->Render();
    }
}
