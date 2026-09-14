#pragma once
#include "Engine/UI/UIContainer.h"
#include "Engine/UI/UIAction.h"
#include <vector>

// 菜单打开时的暂停语义（与 UIManager::Active() 解耦，按菜单定义）
enum class UIPauseMode {
    None,      // 不暂停（如 HUD / 只展示）
    Gameplay,  // 暂停 gameplay 更新（默认）
    Full       // 暂停 gameplay + 帧后效果等
};

// Menu = UIElement 容器 + focus/navigation 状态。不含 input/action 逻辑。
// focus order = children 顺序中的可 focus 项（visible && enabled && focusable）。
class Menu : public UIContainer {
public:
    bool open = false;
    bool active = true;   // false = 被上层菜单覆盖：保留状态但不响应输入、不显示 focus
    UIPauseMode pauseMode = UIPauseMode::Gameplay;

    void Open();     // 显示并自动聚焦第一个有效元素
    void Close();    // 隐藏并清除 focus
    bool IsOpen() const { return open; }
    void SetActive(bool a);
    bool IsActive() const { return active; }

    void FocusNext();
    void FocusPrev();
    void SetFocus(UIElement* element);
    void ClearFocus();
    UIElement* Focused() const { return focus; }

    // ---- action model ----
    // Menu/组件只产出 UIAction；上层用 ConsumeActions()/PendingActions() 取走。
    void ActivateFocused();        // UIConfirm：分发给聚焦元素（无聚焦 -> menu Confirm）
    void AdjustFocused(int dir);   // UILeft(-1)/UIRight(+1)
    void EmitAction(const std::string& elementId, const std::string& action,
                    nlohmann::json value = nlohmann::json());
    std::vector<UIAction> ConsumeActions();                    // move 并清空
    const std::vector<UIAction>& PendingActions() const { return actions; }

    // menu 级默认 action；子类可覆写（不调用基类则自行产生 action）
    virtual void Confirm();
    virtual void Cancel();

    void Update(float dt) override;
    void Render() override;

protected:
    std::vector<UIElement*> ValidFocusables() const;   // visible && enabled && focusable
    void EnsureFocusValid();                            // focus 失效时自动迁移

    UIElement* focus = nullptr;
    std::vector<UIAction> actions;
};
