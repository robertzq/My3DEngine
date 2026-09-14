#include "Engine/UI/UILayout.h"
#include "Engine/UI/UIElement.h"
#include <algorithm>

namespace {

int AlignCross(LayoutCrossAlign a, int contentStart, int contentSize, int childSize) {
    switch (a) {
        case LayoutCrossAlign::Center: return contentStart + (contentSize - childSize) / 2;
        case LayoutCrossAlign::End:    return contentStart + contentSize - childSize;
        default:                       return contentStart;   // Start / Stretch
    }
}

int AlignMain(LayoutMainAlign a, int contentStart, int contentSize, int totalSize) {
    switch (a) {
        case LayoutMainAlign::Center: return contentStart + (contentSize - totalSize) / 2;
        case LayoutMainAlign::End:    return contentStart + contentSize - totalSize;
        default:                      return contentStart;    // Start
    }
}

}

void VerticalLayout::Apply(const SDL_Rect& c, const std::vector<UIElement*>& children) {
    SDL_Rect content{c.x + padding, c.y + padding,
                     c.w - padding * 2, c.h - padding * 2};

    std::vector<UIElement*> vis;
    int total = 0;
    int n = 0;
    for (UIElement* e : children) {
        if (!e || !e->visible) continue;   // invisible 不占空间（disabled 仍占）
        vis.push_back(e);
        total += e->DesiredSize().y;
        ++n;
    }
    if (n > 1) total += spacing * (n - 1);

    int y = AlignMain(mainAlign, content.y, content.h, total);
    for (UIElement* e : vis) {
        SDL_Point d = e->DesiredSize();
        int w = (crossAlign == LayoutCrossAlign::Stretch) ? content.w : d.x;
        int x = AlignCross(crossAlign, content.x, content.w, w);
        e->resolved = SDL_Rect{x, y, w, d.y};
        e->layoutManaged = true;
        y += d.y + spacing;
    }
}

void HorizontalLayout::Apply(const SDL_Rect& c, const std::vector<UIElement*>& children) {
    SDL_Rect content{c.x + padding, c.y + padding,
                     c.w - padding * 2, c.h - padding * 2};

    std::vector<UIElement*> vis;
    int total = 0;
    int n = 0;
    for (UIElement* e : children) {
        if (!e || !e->visible) continue;
        vis.push_back(e);
        total += e->DesiredSize().x;
        ++n;
    }
    if (n > 1) total += spacing * (n - 1);

    int x = AlignMain(mainAlign, content.x, content.w, total);
    for (UIElement* e : vis) {
        SDL_Point d = e->DesiredSize();
        int h = (crossAlign == LayoutCrossAlign::Stretch) ? content.h : d.y;
        int y = AlignCross(crossAlign, content.y, content.h, h);
        e->resolved = SDL_Rect{x, y, d.x, h};
        e->layoutManaged = true;
        x += d.x + spacing;
    }
}
