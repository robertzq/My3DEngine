#pragma once
#include "Engine/UI/UIElement.h"
#include "Engine/TextRenderer.h"

class Texture;

// 文本；不参与 focus
class Label : public UIElement {
public:
    std::string text;
    int fontSize = 0;                 // 0 = 默认字号
    SDL_Color color{235, 235, 235, 255};
    TextAlign hAlign = TextAlign::Left;
    TextVAlign vAlign = TextVAlign::Middle;
    void Render() override;
    SDL_Point DesiredSize() const override;
};

// 图片；texture 为非拥有指针（ResourceManager 拥有）。不参与 focus
class Image : public UIElement {
public:
    const Texture* texture = nullptr;
    SDL_Rect src{0, 0, 0, 0};
    bool flipX = false;
    bool flipY = false;
    void Render() override;
    SDL_Point DesiredSize() const override;
};

// 纯色面板 / 背景；不参与 focus
class Panel : public UIElement {
public:
    SDL_Color color{0, 0, 0, 160};
    bool border = false;
    SDL_Color borderColor{255, 255, 255, 255};
    int borderThickness = 1;
    void Render() override;
};

// 按钮：只持 action 数据，不执行逻辑。focusable
class Button : public UIElement {
public:
    Button() { focusable = true; }
    std::string text;
    std::string action;               // 数据：动作 id（由上层解释）
    int fontSize = 0;
    SDL_Color textColor{235, 235, 235, 255};
    SDL_Color normalColor{40, 40, 52, 200};
    SDL_Color focusedColor{70, 110, 190, 230};
    SDL_Color pressedColor{120, 160, 240, 255};
    SDL_Color disabledColor{60, 60, 60, 160};
    bool focused = false;
    bool pressed = false;
    void Render() override;
    SDL_Point DesiredSize() const override;
};

// 开关：bool value。focusable（切换输入在后续 Step）
class Toggle : public UIElement {
public:
    Toggle() { focusable = true; }
    std::string text;
    bool value = false;
    int fontSize = 0;
    SDL_Color textColor{235, 235, 235, 255};
    SDL_Color onColor{80, 180, 110, 255};
    SDL_Color offColor{110, 110, 120, 255};
    bool focused = false;
    void Render() override;
    SDL_Point DesiredSize() const override;
};

// 滑条：min/max/step/value。focusable（左右调节在后续 Step）
class Slider : public UIElement {
public:
    Slider() { focusable = true; }
    std::string text;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 0.05f;
    float value = 0.0f;
    int fontSize = 0;
    SDL_Color textColor{235, 235, 235, 255};
    SDL_Color trackColor{55, 55, 66, 255};
    SDL_Color fillColor{90, 160, 255, 255};
    SDL_Color knobColor{235, 235, 235, 255};
    bool focused = false;

    void SetValue(float v);
    float Value() const { return value; }
    void Render() override;
    SDL_Point DesiredSize() const override;
};

// 占位（仅参与布局，不绘制）
class Spacer : public UIElement {
public:
    void Render() override {}
};
