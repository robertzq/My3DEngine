#pragma once
#include <SDL.h>
#include <vector>

class UIElement;

// 主轴（堆叠方向）：Start=上/左，Center=中，End=下/右
enum class LayoutMainAlign { Start, Center, End };
// 交叉轴：Start/Center/End/Stretch
enum class LayoutCrossAlign { Start, Center, End, Stretch };

// Layout 只负责：根据容器 rect + children desired size + padding/spacing/alignment，
// 计算并写入每个 child 的 resolved rect。不拥有 children，不做 render/input/focus。
class UILayout {
public:
    virtual ~UILayout() = default;
    virtual void Apply(const SDL_Rect& container, const std::vector<UIElement*>& children) = 0;
};

// 垂直堆叠；main=纵向，cross=横向
class VerticalLayout : public UILayout {
public:
    int spacing = 12;
    int padding = 16;
    LayoutMainAlign mainAlign = LayoutMainAlign::Start;
    LayoutCrossAlign crossAlign = LayoutCrossAlign::Start;
    void Apply(const SDL_Rect& container, const std::vector<UIElement*>& children) override;
};

// 水平堆叠；main=横向，cross=纵向
class HorizontalLayout : public UILayout {
public:
    int spacing = 12;
    int padding = 16;
    LayoutMainAlign mainAlign = LayoutMainAlign::Start;
    LayoutCrossAlign crossAlign = LayoutCrossAlign::Start;
    void Apply(const SDL_Rect& container, const std::vector<UIElement*>& children) override;
};
