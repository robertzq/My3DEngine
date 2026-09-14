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

    SDL_Rect rect{0, 0, 0, 0};   // x,y 相对 anchor 点；w,h 为尺寸
    Anchor anchor = Anchor::TopLeft;
    SDL_Point offset{0, 0};
    float opacity = 1.0f;
    SDL_Color tint{255, 255, 255, 255};

    virtual void Update(float dt) {}
    virtual void Render() = 0;

    // anchor + offset + rect 解析成屏幕像素矩形（依赖当前 Renderer 尺寸）
    SDL_Rect ScreenRect() const;

protected:
    // 应用 tint 与 opacity
    SDL_Color Modulate(const SDL_Color& c) const;
};
