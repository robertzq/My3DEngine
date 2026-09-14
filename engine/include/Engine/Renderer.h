#pragma once
#include <SDL.h>
#include "Engine/Texture.h"

class Shader;
class SpriteEffect;

// 每次 DrawSprite 的可选参数。shader 为空时使用默认 sprite shader。
struct SpriteDrawOptions {
    Shader* shader = nullptr;            // 指定 shader（为空用默认）
    const SpriteEffect* effect = nullptr; // 指定则用其 shader + 参数（优先级高于 shader）
    SDL_Color tint = {255, 255, 255, 255};
    bool flipX = false;
    bool flipY = false;
};

// 最小动态 mesh 顶点：pos(像素) + uv(归一化)。仅服务 Page Curl 等最小需求。
struct MeshVertex {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
};

struct MeshDrawOptions {
    SDL_Color tint = {255, 255, 255, 255};
};

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

    // 渲染目标用的颜色纹理（LINEAR / CLAMP，无 mipmap），由调用方（RenderTarget）拥有
    static Texture* CreateRenderTexture(int width, int height);

    // 绑定默认 framebuffer，并把 viewport 设为窗口 drawable 尺寸
    static void BindScreen();

    // 用给定的 fullscreen shader 绘制整屏 quad（采样 input 到 texture unit 0）
    static void DrawFullscreen(Shader& shader, const Texture* input);

    // src 宽/高 <= 0 时使用整张纹理；dst 为屏幕像素坐标（已含 camera offset）
    static void DrawSprite(const Texture* texture, const SDL_Rect& src, const SDL_Rect& dst,
                           SDL_RendererFlip flip, const SDL_Color& tint = {255, 255, 255, 255});

    // 带 shader / effect 的版本（自动注入内置 uniform）
    static void DrawSprite(const Texture* texture, const SDL_Rect& src, const SDL_Rect& dst,
                           const SpriteDrawOptions& options);

    // 最小动态 mesh：单纹理、pos+uv、一个动态 VBO/EBO/VAO（不每帧重建）
    static void DrawMesh(Shader& shader, const MeshVertex* vertices, int vertexCount,
                         const unsigned short* indices, int indexCount,
                         const Texture* texture, const MeshDrawOptions& options = {});

    // 基础图元（等价旧 SDL_RenderFillRect / DrawRect / DrawLine / RenderClear）
    static void Clear(const SDL_Color& color);
    static void DrawRect(const SDL_Rect& dst, const SDL_Color& color);
    static void DrawRectOutline(const SDL_Rect& dst, const SDL_Color& color, int thickness = 1);
    static void DrawLine(int x1, int y1, int x2, int y2, const SDL_Color& color);

    static bool Ready();
    static int Width();
    static int Height();

    // 调试：开启后 CheckError 才会真正 poll glGetError（生产路径默认关闭）
    static void SetDebug(bool enabled);
    static bool Debug();
    static bool CheckError(const char* context);
};
