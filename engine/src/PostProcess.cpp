#include "Engine/PostProcess.h"
#include "Engine/GL.h"
#include "Engine/Game.h"
#include "Engine/Log.h"
#include "Engine/RenderTarget.h"
#include "Engine/Renderer.h"
#include "Engine/Shader.h"
#include "Engine/Texture.h"
#include "Engine/Time.h"
#include <memory>

namespace {

const char* BLIT_VS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aUnit;\n"
    "out vec2 vUv;\n"
    "void main(){ vUv = aUnit; gl_Position = vec4(aUnit * 2.0 - 1.0, 0.0, 1.0); }\n";

const char* BLIT_FS =
    "#version 330 core\n"
    "in vec2 vUv; out vec4 FragColor;\n"
    "uniform sampler2D u_inputTexture;\n"
    "void main(){ FragColor = texture(u_inputTexture, vUv); }\n";

struct State {
    std::unique_ptr<RenderTarget> world;
    std::unique_ptr<RenderTarget> composite;
    std::unique_ptr<RenderTarget> ping;
    std::unique_ptr<RenderTarget> pong;
    std::unique_ptr<Shader> blit;
    std::vector<PostEffect> worldEffects;
    std::vector<PostEffect> finalEffects;
    int width = 0;
    int height = 0;
    bool initialized = false;
};

State& S() {
    static State s;
    return s;
}

void SetBuiltIns(Shader& shader, int w, int h, float effectProgress) {
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    shader.SetFloat("u_time", static_cast<float>(Time::ElapsedTime()));
    shader.SetFloat("u_deltaTime", Time::DeltaTime());
    shader.SetVec2("u_resolution", static_cast<float>(w), static_cast<float>(h));
    shader.SetVec2("u_texelSize", 1.0f / w, 1.0f / h);
    shader.SetVec2("u_cameraPosition", static_cast<float>(Game::camera.x), static_cast<float>(Game::camera.y));
    shader.SetFloat("u_effectProgress", effectProgress);
    shader.SetInt("u_inputTexture", 0);
}

void ApplyParams(Shader& shader, const PostEffect& effect) {
    for (const auto& entry : effect.params) {
        const PostEffect::Param& p = entry.second;
        switch (p.kind) {
            case 1: shader.SetInt(entry.first, p.i); break;
            case 2: shader.SetFloat(entry.first, p.f[0]); break;
            case 3: shader.SetVec2(entry.first, p.f[0], p.f[1]); break;
            case 4: shader.SetVec3(entry.first, p.f[0], p.f[1], p.f[2]); break;
            case 5: shader.SetVec4(entry.first, p.f[0], p.f[1], p.f[2], p.f[3]); break;
            default: break;
        }
    }
}

void BindOutput(RenderTarget* target) {
    if (target) target->Bind();
    else Renderer::BindScreen();
}

// input -> (effects) -> target(RT or screen)
void RunChain(const Texture* input, std::vector<PostEffect>& effects, RenderTarget* target,
              const char* label) {
    std::vector<PostEffect*> enabled;
    for (auto& e : effects) {
        if (e.enabled && e.shader && e.shader->Valid()) enabled.push_back(&e);
    }

    if (enabled.empty()) {
        if (!S().blit) return;
        BindOutput(target);
        int w = target ? target->Width() : Renderer::Width();
        int h = target ? target->Height() : Renderer::Height();
        S().blit->Use();
        SetBuiltIns(*S().blit, w, h, 0.0f);
        Renderer::DrawFullscreen(*S().blit, input);
        Renderer::CheckError("postprocess blit");
        return;
    }

    const Texture* current = input;
    const int count = static_cast<int>(enabled.size());
    for (int i = 0; i < count; ++i) {
        PostEffect& effect = *enabled[i];
        const bool last = (i == count - 1);
        RenderTarget* out = last ? target : (i % 2 == 0 ? S().ping.get() : S().pong.get());
        if (!last && !out) break;

        BindOutput(out);
        int w = out ? out->Width() : Renderer::Width();
        int h = out ? out->Height() : Renderer::Height();
        effect.shader->Use();
        SetBuiltIns(*effect.shader, w, h, effect.progress);
        ApplyParams(*effect.shader, effect);
        Renderer::DrawFullscreen(*effect.shader, current);
        Renderer::CheckError(effect.name.empty() ? label : effect.name.c_str());

        if (!last) current = out->ColorTexture();
    }
}

}

void PostEffect::SetInt(const std::string& n, int value) {
    Param p; p.kind = 1; p.i = value; params[n] = p;
}
void PostEffect::SetFloat(const std::string& n, float value) {
    Param p; p.kind = 2; p.f[0] = value; params[n] = p;
}
void PostEffect::SetVec2(const std::string& n, float x, float y) {
    Param p; p.kind = 3; p.f[0] = x; p.f[1] = y; params[n] = p;
}
void PostEffect::SetVec3(const std::string& n, float x, float y, float z) {
    Param p; p.kind = 4; p.f[0] = x; p.f[1] = y; p.f[2] = z; params[n] = p;
}
void PostEffect::SetVec4(const std::string& n, float x, float y, float z, float w) {
    Param p; p.kind = 5; p.f[0] = x; p.f[1] = y; p.f[2] = z; p.f[3] = w; params[n] = p;
}

bool PostProcess::Init() {
    State& s = S();
    if (s.initialized) return true;

    s.blit = std::make_unique<Shader>();
    if (!s.blit->LoadFromSource(BLIT_VS, BLIT_FS, "postprocess_blit")) {
        LOG_ERROR("PostProcess: blit shader 编译失败");
        s.blit.reset();
        return false;
    }

    s.initialized = true;
    LOG_INFO("PostProcess 初始化完成");
    return true;
}

void PostProcess::Clean() {
    State& s = S();
    s.world.reset();
    s.composite.reset();
    s.ping.reset();
    s.pong.reset();
    s.blit.reset();
    s.worldEffects.clear();
    s.finalEffects.clear();
    s.width = s.height = 0;
    s.initialized = false;
}

void PostProcess::Resize(int width, int height) {
    State& s = S();
    if (width <= 0 || height <= 0) return;
    if (s.width == width && s.height == height &&
        s.world && s.composite && s.ping && s.pong) return;

    if (!s.world) s.world = std::make_unique<RenderTarget>();
    if (!s.composite) s.composite = std::make_unique<RenderTarget>();
    if (!s.ping) s.ping = std::make_unique<RenderTarget>();
    if (!s.pong) s.pong = std::make_unique<RenderTarget>();

    s.world->Resize(width, height);
    s.composite->Resize(width, height);
    s.ping->Resize(width, height);
    s.pong->Resize(width, height);

    s.width = width;
    s.height = height;
}

void PostProcess::BeginWorld() {
    State& s = S();
    if (!s.world || !s.world->Valid()) {
        LOG_ERROR("PostProcess: BeginWorld 但 world FBO 未就绪");
        return;
    }
    s.world->Bind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    Renderer::CheckError("postprocess BeginWorld");
}

void PostProcess::EndWorld() {
    Renderer::BindScreen();
}

void PostProcess::AddWorldEffect(const PostEffect& effect) { S().worldEffects.push_back(effect); }
void PostProcess::AddFinalEffect(const PostEffect& effect) { S().finalEffects.push_back(effect); }

void PostProcess::ClearEffects() {
    S().worldEffects.clear();
    S().finalEffects.clear();
}
void PostProcess::ClearWorldEffects() { S().worldEffects.clear(); }
void PostProcess::ClearFinalEffects() { S().finalEffects.clear(); }

void PostProcess::ApplyWorld() {
    State& s = S();
    if (!s.world || !s.world->Valid() || !s.composite || !s.composite->Valid()) return;
    RunChain(s.world->ColorTexture(), s.worldEffects, s.composite.get(), "world");
}

void PostProcess::ApplyFinal() {
    State& s = S();
    if (!s.composite || !s.composite->Valid()) return;
    RunChain(s.composite->ColorTexture(), s.finalEffects, nullptr, "final");
}

void PostProcess::BindComposite() {
    State& s = S();
    if (s.composite && s.composite->Valid()) s.composite->Bind();
}

const Texture* PostProcess::CompositeTexture() {
    State& s = S();
    return (s.composite && s.composite->Valid()) ? s.composite->ColorTexture() : nullptr;
}

const Texture* PostProcess::WorldTexture() {
    State& s = S();
    return (s.world && s.world->Valid()) ? s.world->ColorTexture() : nullptr;
}
