#pragma once
#include "Engine/UI/UIContainer.h"

// Menu = UIElement 容器 + focus/navigation 状态。不含 input/action 逻辑。
// focus order = children 顺序中的可 focus 项（visible && enabled && focusable）。
class Menu : public UIContainer {
public:
    bool open = false;

    void Open();     // 显示并自动聚焦第一个有效元素
    void Close();    // 隐藏并清除 focus
    bool IsOpen() const { return open; }

    void FocusNext();
    void FocusPrev();
    void SetFocus(UIElement* element);
    void ClearFocus();
    UIElement* Focused() const { return focus; }

    // 本 Step 仅提供 hook；action 分发/输入路由在后续 Step
    virtual void Confirm() {}
    virtual void Cancel() {}

    void Update(float dt) override;
    void Render() override;

protected:
    std::vector<UIElement*> ValidFocusables() const;   // visible && enabled && focusable
    void EnsureFocusValid();                            // focus 失效时自动迁移

    UIElement* focus = nullptr;
};
