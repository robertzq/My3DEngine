#include "Engine/UI/UIElement.h"
#include "Engine/Renderer.h"

SDL_Rect UIElement::ScreenRect() const {
    if (layoutManaged) return resolved;

    const int W = Renderer::Width();
    const int H = Renderer::Height();
    float fx = 0.0f, fy = 0.0f;   // 元素自身的锚点在屏幕上的比例位置
    switch (anchor) {
        case Anchor::TopLeft:      fx = 0.0f; fy = 0.0f; break;
        case Anchor::TopCenter:    fx = 0.5f; fy = 0.0f; break;
        case Anchor::TopRight:     fx = 1.0f; fy = 0.0f; break;
        case Anchor::CenterLeft:   fx = 0.0f; fy = 0.5f; break;
        case Anchor::Center:       fx = 0.5f; fy = 0.5f; break;
        case Anchor::CenterRight:  fx = 1.0f; fy = 0.5f; break;
        case Anchor::BottomLeft:   fx = 0.0f; fy = 1.0f; break;
        case Anchor::BottomCenter: fx = 0.5f; fy = 1.0f; break;
        case Anchor::BottomRight:  fx = 1.0f; fy = 1.0f; break;
    }
    SDL_Rect r;
    r.x = static_cast<int>(fx * (W - rect.w)) + offset.x + rect.x;
    r.y = static_cast<int>(fy * (H - rect.h)) + offset.y + rect.y;
    r.w = rect.w;
    r.h = rect.h;
    return r;
}

SDL_Color UIElement::Modulate(const SDL_Color& c) const {
    SDL_Color o;
    o.r = static_cast<Uint8>(c.r * tint.r / 255);
    o.g = static_cast<Uint8>(c.g * tint.g / 255);
    o.b = static_cast<Uint8>(c.b * tint.b / 255);
    o.a = static_cast<Uint8>(c.a * tint.a / 255 * opacity);
    return o;
}
