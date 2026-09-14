#pragma once
#include <map>
#include <string>
#include <vector>

class Shader;
class Texture;

// 单个 post effect 实例：shader 指针 + 实例自己的参数。
// 同一个 shader 可以创建多个实例，参数互不影响。
struct PostEffect {
    std::string name;
    Shader* shader = nullptr;   // non-owning，由创建者保证存活
    bool enabled = true;
    float progress = 0.0f;      // 自动注入 u_effectProgress

    void SetInt(const std::string& n, int value);
    void SetFloat(const std::string& n, float value);
    void SetVec2(const std::string& n, float x, float y);
    void SetVec3(const std::string& n, float x, float y, float z);
    void SetVec4(const std::string& n, float x, float y, float z, float w);

    // kind: 1=int 2=float 3=vec2 4=vec3 5=vec4
    struct Param { int kind = 0; int i = 0; float f[4] = {0, 0, 0, 0}; };
    std::map<std::string, Param> params;
};

// 引擎级 post-process 管线（静态子系统，由 Game 驱动）。
//
//   World FBO -> WorldEffects -> composite -> (UI) -> FinalEffects -> Screen
//
// * 0 effect：直接 fullscreen blit。
// * N effect：ping-pong render target 顺序执行；disabled 跳过。
// * FBO / shader 不每帧重建；内置 uniform 自动注入。
class PostProcess {
public:
    static bool Init();
    static void Clean();

    static void Resize(int width, int height);

    // World pass：场景渲染进 world FBO
    static void BeginWorld();
    static void EndWorld();

    static void AddWorldEffect(const PostEffect& effect);
    static void AddFinalEffect(const PostEffect& effect);
    static void ClearEffects();
    static void ClearWorldEffects();
    static void ClearFinalEffects();

    static void ApplyWorld();   // world FBO -> (world effects) -> composite
    static void ApplyFinal();   // composite -> (final effects) -> screen

    // 供未来 UI 使用：把 UI 画到 composite 上（FinalFX 之前）
    static void BindComposite();
    static const Texture* CompositeTexture();
    static const Texture* WorldTexture();
};
