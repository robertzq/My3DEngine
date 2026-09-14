#pragma once
#include <SDL.h>
#include "Engine/Texture.h"

// OpenGL 3.3 Core 2D 渲染后端。
// 坐标语义保持和旧 SDL_Renderer 一致：origin 左上、+x 右、+y 下，单位为像素。
// 相机偏移仍由调用方（CPU 端）应用；本类只负责像素坐标 -> NDC 的转换。
class Renderer {
public:
    // 必须在 SDL_CreateWindow 之前调用（设置 GL context 属性）
    static void SetAttributes();

    // 在窗口创建后调用：建立 GL context、加载 GL 函数、编译默认 sprite shader
    static bool Init(SDL_Window* window);
    static void Clean();

    static void BeginFrame(int width, int height);
    static void EndFrame();

    // 纹理所有权：创建后由调用方（ResourceManager / TextRenderer）负责，
    // 必须通过 DestroyTexture 在 GL context 有效期间释放。
    static Texture* CreateTextureFromSurface(SDL_Surface* surface);
    static void DestroyTexture(Texture* texture);

    // src 宽/高 <= 0 时使用整张纹理；dst 为屏幕像素坐标（已含 camera offset）
    static void DrawSprite(const Texture* texture, const SDL_Rect& src, const SDL_Rect& dst,
                           SDL_RendererFlip flip, const SDL_Color& tint = {255, 255, 255, 255});

    // 基础图元（等价旧 SDL_RenderFillRect / DrawRect / DrawLine / RenderClear）
    static void Clear(const SDL_Color& color);
    static void DrawRect(const SDL_Rect& dst, const SDL_Color& color);
    static void DrawRectOutline(const SDL_Rect& dst, const SDL_Color& color, int thickness = 1);
    static void DrawLine(int x1, int y1, int x2, int y2, const SDL_Color& color);

    static bool Ready();
    static int Width();
    static int Height();
};
