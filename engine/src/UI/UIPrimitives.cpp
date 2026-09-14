#include "Engine/UI/UIPrimitives.h"
#include "Engine/Renderer.h"
#include "Engine/Texture.h"
#include <algorithm>
#include <cmath>

void Label::Render() {
    if (!visible) return;
    TextRenderer::DrawTextIn(ScreenRect(), text, Modulate(color), fontSize, hAlign, vAlign);
}

void Image::Render() {
    if (!visible || !texture) return;
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (flipX) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
    if (flipY) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);
    Renderer::DrawSprite(texture, src, ScreenRect(), flip, Modulate(tint));
}

void Panel::Render() {
    if (!visible) return;
    SDL_Rect box = ScreenRect();
    Renderer::DrawRect(box, Modulate(color));
    if (border) Renderer::DrawRectOutline(box, Modulate(borderColor), borderThickness);
}

void Button::Render() {
    if (!visible) return;
    SDL_Rect box = ScreenRect();
    SDL_Color base = normalColor;
    if (!enabled) base = disabledColor;
    else if (pressed) base = pressedColor;
    else if (focused) base = focusedColor;

    Renderer::DrawRect(box, Modulate(base));
    TextRenderer::DrawTextIn(box, text, Modulate(textColor), fontSize,
                             TextAlign::Center, TextVAlign::Middle);
    if (focused && enabled) {
        Renderer::DrawRectOutline(box, Modulate(SDL_Color{255, 255, 255, 180}), 1);
    }
}

void Toggle::Render() {
    if (!visible) return;
    SDL_Rect box = ScreenRect();

    SDL_Rect labelBox = {box.x, box.y, box.w - 68, box.h};
    TextRenderer::DrawTextIn(labelBox, text, Modulate(textColor), fontSize,
                             TextAlign::Left, TextVAlign::Middle);

    SDL_Rect ind = {box.x + box.w - 60, box.y + box.h / 2 - 12, 56, 24};
    SDL_Color indColor = !enabled ? SDL_Color{70, 70, 70, 200} : (value ? onColor : offColor);
    Renderer::DrawRect(ind, Modulate(indColor));
    TextRenderer::DrawTextIn(ind, value ? "ON" : "OFF", Modulate(SDL_Color{255, 255, 255, 255}),
                             fontSize, TextAlign::Center, TextVAlign::Middle);
    if (focused && enabled) Renderer::DrawRectOutline(box, Modulate(SDL_Color{255, 255, 255, 180}), 1);
}

void Slider::SetValue(float v) {
    if (maxValue < minValue) std::swap(maxValue, minValue);
    if (step > 0.0f) {
        v = minValue + std::round((v - minValue) / step) * step;
    }
    value = std::min(std::max(v, minValue), maxValue);
}

void Slider::Render() {
    if (!visible) return;
    SDL_Rect box = ScreenRect();

    TextRenderer::DrawTextIn({box.x, box.y, box.w, 24}, text, Modulate(textColor), fontSize,
                             TextAlign::Left, TextVAlign::Middle);

    SDL_Rect track = {box.x, box.y + box.h - 12, box.w, 8};
    Renderer::DrawRect(track, Modulate(trackColor));

    float t = (maxValue > minValue) ? (value - minValue) / (maxValue - minValue) : 0.0f;
    t = std::min(std::max(t, 0.0f), 1.0f);
    int fillW = static_cast<int>(track.w * t);
    Renderer::DrawRect({track.x, track.y, fillW, track.h}, Modulate(fillColor));

    SDL_Rect knob = {track.x + fillW - 4, track.y - 5, 8, 18};
    Renderer::DrawRect(knob, Modulate(knobColor));

    if (focused && enabled) Renderer::DrawRectOutline(box, Modulate(SDL_Color{255, 255, 255, 160}), 1);
}

// ---------------- DesiredSize ----------------

SDL_Point Label::DesiredSize() const {
    return TextRenderer::MeasureText(text, fontSize);
}

SDL_Point Image::DesiredSize() const {
    if (rect.w > 0 && rect.h > 0) return {rect.w, rect.h};
    if (texture) return {texture->Width(), texture->Height()};
    return {rect.w, rect.h};
}

SDL_Point Button::DesiredSize() const {
    SDL_Point m = TextRenderer::MeasureText(text, fontSize);
    int w = rect.w > 0 ? rect.w : m.x + 40;
    int h = rect.h > 0 ? rect.h : m.y + 20;
    return {w, h};
}

SDL_Point Toggle::DesiredSize() const {
    SDL_Point m = TextRenderer::MeasureText(text, fontSize);
    int w = rect.w > 0 ? rect.w : m.x + 68 + 24;
    int h = rect.h > 0 ? rect.h : std::max(m.y, 24) + 12;
    return {w, h};
}

SDL_Point Slider::DesiredSize() const {
    int w = rect.w > 0 ? rect.w : 220;
    int h = rect.h > 0 ? rect.h : 44;
    return {w, h};
}
