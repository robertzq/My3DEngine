#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Engine/UI/UIElement.h"
#include "Engine/UI/UILayout.h"

// 最小 UI 容器：拥有 children（unique_ptr），可选布局。children 非 layout 拥有。
class UIContainer : public UIElement {
public:
    std::vector<std::unique_ptr<UIElement>> children;
    std::unique_ptr<UILayout> layout;

    UIElement* Add(std::unique_ptr<UIElement> child);
    UIElement* Find(const std::string& id) const;

    // 用当前 ScreenRect 作为容器矩形重排 children（resize 后每帧调用即可）
    void ApplyLayout();

    void Update(float dt) override;   // 先更新 children，再重排
    void Render() override;           // 依次渲染可见 children

    SDL_Point DesiredSize() const override { return {rect.w, rect.h}; }
};
