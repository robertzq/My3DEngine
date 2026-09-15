#include "Engine/Renderer.h"
#include "Engine/GL.h"
#include "Engine/Shader.h"
#include "Engine/ShaderManager.h"
#include "Engine/SpriteEffect.h"
#include "Engine/Game.h"
#include "Engine/Log.h"
#include "Engine/Time.h"
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include <vector>

namespace {

SDL_Window* window = nullptr;
SDL_GLContext context = nullptr;

Shader* spriteShader = nullptr;   // non-owning，由 ShaderManager 持有
unsigned int quadVao = 0;
unsigned int quadVbo = 0;
unsigned int meshVao = 0;
unsigned int meshVbo = 0;
unsigned int meshEbo = 0;
Shader* meshShader = nullptr;
Texture whiteTexture;   // 1x1 白色，用于图元绘制

int viewportWidth = 0;
int viewportHeight = 0;
bool ready = false;
bool debug = false;

const char* SPRITE_VS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aUnit;      // 0..1 quad\n"
    "uniform vec4 uDstRect;                 // x,y,w,h (pixels)\n"
    "uniform vec4 uUvRect;                  // u,v,uw,uh (normalized)\n"
    "uniform vec2 u_resolution;\n"
    "uniform vec2 uFlip;                    // 1.0 = flip axis\n"
    "out vec2 vUv;\n"
    "void main() {\n"
    "    vec2 uv = uUvRect.xy + aUnit * uUvRect.zw;\n"
    "    if (uFlip.x > 0.5) uv.x = uUvRect.x + uUvRect.z - (uv.x - uUvRect.x);\n"
    "    if (uFlip.y > 0.5) uv.y = uUvRect.y + uUvRect.w - (uv.y - uUvRect.y);\n"
    "    vUv = uv;\n"
    "    vec2 px = uDstRect.xy + aUnit * uDstRect.zw;\n"
    "    float ndcX = (px.x / u_resolution.x) * 2.0 - 1.0;\n"
    "    float ndcY = 1.0 - (px.y / u_resolution.y) * 2.0;\n"
    "    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);\n"
    "}\n";

const char* SPRITE_FS =
    "#version 330 core\n"
    "in vec2 vUv;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D u_texture;\n"
    "uniform vec4 u_tint;\n"
    "void main() {\n"
    "    FragColor = texture(u_texture, vUv) * u_tint;\n"
    "}\n";

const float UNIT_QUAD[12] = {
    0.f, 0.f,
    1.f, 0.f,
    1.f, 1.f,
    0.f, 0.f,
    1.f, 1.f,
    0.f, 1.f,
};

void EnsureQuad() {
    if (quadVao != 0) return;
    glGenVertexArrays(1, &quadVao);
    glGenBuffers(1, &quadVbo);
    glBindVertexArray(quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(UNIT_QUAD), UNIT_QUAD, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void EnsureMesh() {
    if (meshVao != 0) return;
    glGenVertexArrays(1, &meshVao);
    glGenBuffers(1, &meshVbo);
    glGenBuffers(1, &meshEbo);
    glBindVertexArray(meshVao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glBindVertexArray(0);
}

void SetBuiltInUniforms(Shader& shader, const SDL_Color& tint) {
    const float vw = (float)(viewportWidth > 0 ? viewportWidth : 1);
    const float vh = (float)(viewportHeight > 0 ? viewportHeight : 1);
    shader.SetFloat("u_time", static_cast<float>(Time::ElapsedTime()));
    shader.SetFloat("u_deltaTime", Time::DeltaTime());
    shader.SetVec2("u_resolution", vw, vh);
    shader.SetVec2("u_texelSize", 1.0f / vw, 1.0f / vh);
    shader.SetVec2("u_cameraPosition", (float)Game::camera.x, (float)Game::camera.y);
    shader.SetVec4("u_tint", tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f);
    shader.SetInt("u_texture", 0);
}

}

void Renderer::SetAttributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

bool Renderer::Init(SDL_Window* win) {
    window = win;
    context = SDL_GL_CreateContext(window);
    if (!context) {
        LOG_ERROR("Renderer: SDL_GL_CreateContext 失败: " << SDL_GetError());
        return false;
    }

    if (!LoadGLFunctions()) {
        LOG_ERROR("Renderer: OpenGL 函数加载失败");
        return false;
    }

    LOG_INFO("Renderer: GL_VERSION=" << (const char*)glGetString(0x1F02)
             << " RENDERER=" << (const char*)glGetString(0x1F01));

    // 等距 mesh 专用 shader：顶点坐标直接是屏幕像素（不是 0..1 quad），
    // 由 shader 内统一转 NDC；UV 直接是纹理归一坐标，无 uDstRect/uUvRect 包装。
    static const char* MESH_VS =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "layout(location=1) in vec2 aUv;\n"
        "uniform vec2 u_resolution;\n"
        "out vec2 vUv;\n"
        "void main() {\n"
        "    vUv = aUv;\n"
        "    float ndcX = (aPos.x / u_resolution.x) * 2.0 - 1.0;\n"
        "    float ndcY = 1.0 - (aPos.y / u_resolution.y) * 2.0;\n"
        "    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);\n"
        "}\n";
    static const char* MESH_FS =
        "#version 330 core\n"
        "in vec2 vUv;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D u_texture;\n"
        "uniform vec4 u_tint;\n"
        "void main() {\n"
        "    vec4 tex = texture(u_texture, vUv);\n"
        "    FragColor = tex * u_tint;\n"
        "}\n";
    meshShader = ShaderManager::Register("mesh_default", MESH_VS, MESH_FS);
    if (!meshShader) {
        LOG_ERROR("Renderer: 默认 mesh shader 编译失败");
        return false;
    }
    spriteShader = ShaderManager::Register("sprite_default", SPRITE_VS, SPRITE_FS);
    if (!spriteShader) {
        LOG_ERROR("Renderer: 默认 sprite shader 编译失败");
        return false;
    }

    EnsureQuad();
    EnsureMesh();

    // 1x1 白色纹理，用于填充矩形 / 线条
    const unsigned char white[4] = {255, 255, 255, 255};
    whiteTexture.width_ = 1;
    whiteTexture.height_ = 1;
    glGenTextures(1, &whiteTexture.id_);
    glBindTexture(GL_TEXTURE_2D, whiteTexture.id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    ready = true;
    return true;
}

void Renderer::Clean() {
    spriteShader = nullptr;   // Shader 由 ShaderManager 释放
    if (whiteTexture.id_) { glDeleteTextures(1, &whiteTexture.id_); whiteTexture.id_ = 0; }
    if (meshEbo) { glDeleteBuffers(1, &meshEbo); meshEbo = 0; }
    if (meshVbo) { glDeleteBuffers(1, &meshVbo); meshVbo = 0; }
    if (meshVao) { glDeleteVertexArrays(1, &meshVao); meshVao = 0; }
    if (quadVbo) { glDeleteBuffers(1, &quadVbo); quadVbo = 0; }
    if (quadVao) { glDeleteVertexArrays(1, &quadVao); quadVao = 0; }
    if (context) { SDL_GL_DeleteContext(context); context = nullptr; }
    window = nullptr;
    ready = false;
}

void Renderer::BeginFrame(int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    glViewport(0, 0, width, height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::EndFrame() {
    if (window) SDL_GL_SwapWindow(window);
}

Texture* Renderer::CreateTextureFromSurface(SDL_Surface* surface) {
    if (!surface) return nullptr;

    SDL_Surface* conv = surface;
    bool freeConv = false;
    if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
        conv = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        freeConv = true;
    }
    if (!conv) {
        LOG_ERROR("Renderer: 纹理格式转换失败: " << SDL_GetError());
        return nullptr;
    }

    // 拷贝成紧凑的 RGBA 缓冲，避免 pitch 对齐问题
    const int rowBytes = conv->w * 4;
    std::vector<unsigned char> pixels(static_cast<size_t>(rowBytes) * conv->h);
    for (int y = 0; y < conv->h; ++y) {
        std::memcpy(pixels.data() + static_cast<size_t>(y) * rowBytes,
                    static_cast<unsigned char*>(conv->pixels) + static_cast<size_t>(y) * conv->pitch,
                    rowBytes);
    }
    if (freeConv) SDL_FreeSurface(conv);

    Texture* texture = new Texture();
    texture->width_ = rowBytes / 4;
    texture->height_ = surface->h;

    glGenTextures(1, &texture->id_);
    glBindTexture(GL_TEXTURE_2D, texture->id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->width_, texture->height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    LOG_INFO("Renderer: 纹理上传成功 " << texture->width_ << "x" << texture->height_);
    return texture;
}

void Renderer::DestroyTexture(Texture* texture) {
    if (!texture) return;
    if (texture->id_) glDeleteTextures(1, &texture->id_);
    delete texture;
}

Texture* Renderer::CreateRenderTexture(int width, int height) {
    if (width <= 0 || height <= 0) return nullptr;

    Texture* texture = new Texture();
    texture->width_ = width;
    texture->height_ = height;
    glGenTextures(1, &texture->id_);
    glBindTexture(GL_TEXTURE_2D, texture->id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void Renderer::BindScreen() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
}

void Renderer::DrawFullscreen(Shader& shader, const Texture* input) {
    if (!ready) return;
    shader.Use();
    shader.SetInt("u_inputTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input && input->Valid() ? input->id_ : whiteTexture.id_);
    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::SetDebug(bool enabled) { debug = enabled; }
bool Renderer::Debug() { return debug; }

bool Renderer::CheckError(const char* context) {
    if (!debug) return false;
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERROR("GL error 0x" << std::hex << err << std::dec << " @ " << (context ? context : "?"));
        return true;
    }
    return false;
}

void Renderer::DrawSprite(const Texture* texture, const SDL_Rect& src, const SDL_Rect& dst,
                          SDL_RendererFlip flip, const SDL_Color& tint) {
    SpriteDrawOptions options;
    options.tint = tint;
    options.flipX = (flip & SDL_FLIP_HORIZONTAL) != 0;
    options.flipY = (flip & SDL_FLIP_VERTICAL) != 0;
    DrawSprite(texture, src, dst, options);
}

void Renderer::DrawSprite(const Texture* texture, const SDL_Rect& src, const SDL_Rect& dst,
                          const SpriteDrawOptions& options) {
    if (!ready || !texture || !texture->Valid() || dst.w <= 0 || dst.h <= 0) return;

    SDL_Rect s = src;
    if (s.w <= 0 || s.h <= 0) s = {0, 0, texture->Width(), texture->Height()};
    if (s.w <= 0 || s.h <= 0) return;

    // 选择 shader：effect 优先 -> options.shader -> 默认；无效则回退默认
    Shader* shader = nullptr;
    if (options.effect && options.effect->GetShader()) shader = options.effect->GetShader();
    else if (options.shader) shader = options.shader;
    if (!shader || !shader->Valid()) shader = spriteShader;
    if (!shader || !shader->Valid()) return;

    const float texW = (float)texture->Width();
    const float texH = (float)texture->Height();

    shader->Use();

    // sprite 通道几何 uniform
    shader->SetVec4("uDstRect", (float)dst.x, (float)dst.y, (float)dst.w, (float)dst.h);
    shader->SetVec4("uUvRect", s.x / texW, s.y / texH, s.w / texW, s.h / texH);
    shader->SetVec2("uFlip", options.flipX ? 1.0f : 0.0f, options.flipY ? 1.0f : 0.0f);

    // 内置 uniform
    SetBuiltInUniforms(*shader, options.tint);

    // effect 实例参数
    if (options.effect) {
        for (const auto& entry : options.effect->Params()) {
            const SpriteEffect::Param& p = entry.second;
            switch (p.kind) {
                case 1: shader->SetInt(entry.first, p.i); break;
                case 2: shader->SetFloat(entry.first, p.f[0]); break;
                case 3: shader->SetVec2(entry.first, p.f[0], p.f[1]); break;
                case 4: shader->SetVec3(entry.first, p.f[0], p.f[1], p.f[2]); break;
                case 5: shader->SetVec4(entry.first, p.f[0], p.f[1], p.f[2], p.f[3]); break;
                default: break;
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id_);
    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::DrawMesh(Shader& shader, const MeshVertex* vertices, int vertexCount,
                        const unsigned short* indices, int indexCount,
                        const Texture* texture, const MeshDrawOptions& options) {
    if (!ready || !vertices || vertexCount <= 0 || !indices || indexCount <= 0) return;

    shader.Use();
    SetBuiltInUniforms(shader, options.tint);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture && texture->Valid() ? texture->id_ : whiteTexture.id_);

    glBindVertexArray(meshVao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertexCount * (int)sizeof(MeshVertex)), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indexCount * (int)sizeof(unsigned short)), indices, GL_DYNAMIC_DRAW);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void*)0);
    glBindVertexArray(0);
}

void Renderer::Clear(const SDL_Color& color) {
    if (!ready) return;
    glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::DrawRect(const SDL_Rect& dst, const SDL_Color& color) {
    DrawSprite(&whiteTexture, SDL_Rect{0, 0, 0, 0}, dst, SDL_FLIP_NONE, color);
}

void Renderer::DrawRectOutline(const SDL_Rect& r, const SDL_Color& color, int thickness) {
    if (thickness < 1) thickness = 1;
    DrawRect({r.x, r.y, r.w, thickness}, color);                          // top
    DrawRect({r.x, r.y + r.h - thickness, r.w, thickness}, color);       // bottom
    DrawRect({r.x, r.y, thickness, r.h}, color);                          // left
    DrawRect({r.x + r.w - thickness, r.y, thickness, r.h}, color);        // right
}

void Renderer::DrawLine(int x1, int y1, int x2, int y2, const SDL_Color& color) {
    if (y1 == y2) {
        int x = x1 < x2 ? x1 : x2;
        int w = x1 < x2 ? x2 - x1 : x1 - x2;
        DrawRect({x, y1, w <= 0 ? 1 : w, 1}, color);
    } else if (x1 == x2) {
        int y = y1 < y2 ? y1 : y2;
        int h = y1 < y2 ? y2 - y1 : y1 - y2;
        DrawRect({x1, y, 1, h <= 0 ? 1 : h}, color);
    } else {
        LOG_WARN("Renderer::DrawLine 只支持水平/垂直，已忽略斜线");
    }
}

bool Renderer::Ready() { return ready; }
int Renderer::Width() { return viewportWidth; }
int Renderer::Height() { return viewportHeight; }
