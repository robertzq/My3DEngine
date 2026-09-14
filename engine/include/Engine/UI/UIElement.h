#pragma once
#include <SDL.h>
#include <string>

// 对齐锚点（9 宫格）。位置在使用时按当前窗口尺寸解析。
enum class Anchor {
    TopLeft, TopCenter, TopRight,
    CenterLeft, Center, CenterRight,
    BottomLeft, BottomCenter, BottomRight
};

// 所有 UI 组件的基类：只描述“显示什么”，不含任何 gameplay 逻辑。
class UIElement {
public:
    UIElement() = default;
    virtual ~UIElement() = default;
    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;

    std::string id;

    bool visible = true;
    bool enabled = true;
    bool focusable = false;   // 仅可交互组件为 true（Step 3 才会用到）

    SDL_Rect rect{0, 0, 0, 0};   // x,y 相对 anchor 点；w,h 为尺寸（standalone 用）
    Anchor anchor = Anchor::TopLeft;
    SDL_Point offset{0, 0};
    float opacity = 1.0f;
    SDL_Color tint{255, 255, 255, 255};

    // ---- Layout 契约 ----
    // 被 layout 管理时，layout 把最终屏幕矩形写进 resolved 并置 layoutManaged=true。
    // 此时 anchor/offset/rect 不再参与定位（避免 anchor 与 layout 抢控制权）。
    SDL_Rect resolved{0, 0, 0, 0};
    bool layoutManaged = false;

    // 自然尺寸（layout 用）。默认返回 rect.w/h，组件可覆写（如按文本测量）。
    virtual SDL_Point DesiredSize() const { return {rect.w, rect.h}; }

    virtual void Update(float dt) {}
    virtual void Render() = 0;
    // 由 Menu 设置 focus 视觉状态（可交互组件覆写）；默认无视觉
    virtual void SetFocused(bool focused) {}

    // standalone：anchor + offset + rect；layoutManaged：直接返回 resolved
    SDL_Rect ScreenRect() const;

protected:
    // 应用 tint 与 opacity
    SDL_Color Modulate(const SDL_Color& c) const;
};
