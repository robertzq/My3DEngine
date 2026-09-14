#include "Engine/Renderer.h"
#include "Engine/GL.h"
#include "Engine/Shader.h"
#include "Engine/Log.h"
#include <cstring>
#include <vector>

namespace {

SDL_Window* window = nullptr;
SDL_GLContext context = nullptr;

Shader spriteShader;
unsigned int quadVao = 0;
unsigned int quadVbo = 0;
Texture whiteTexture;   // 1x1 白色，用于图元绘制

int viewportWidth = 0;
int viewportHeight = 0;
bool ready = false;

const char* SPRITE_VS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aUnit;      // 0..1 quad\n"
    "uniform vec4 uDstRect;                 // x,y,w,h (pixels)\n"
    "uniform vec4 uUvRect;                  // u,v,uw,uh (normalized)\n"
    "uniform vec2 uResolution;\n"
    "uniform vec2 uFlip;                    // 1.0 = flip axis\n"
    "out vec2 vUv;\n"
    "void main() {\n"
    "    vec2 uv = uUvRect.xy + aUnit * uUvRect.zw;\n"
    "    if (uFlip.x > 0.5) uv.x = uUvRect.x + uUvRect.z - (uv.x - uUvRect.x);\n"
    "    if (uFlip.y > 0.5) uv.y = uUvRect.y + uUvRect.w - (uv.y - uUvRect.y);\n"
    "    vUv = uv;\n"
    "    vec2 px = uDstRect.xy + aUnit * uDstRect.zw;\n"
    "    float ndcX = (px.x / uResolution.x) * 2.0 - 1.0;\n"
    "    float ndcY = 1.0 - (px.y / uResolution.y) * 2.0;\n"
    "    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);\n"
    "}\n";

const char* SPRITE_FS =
    "#version 330 core\n"
    "in vec2 vUv;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec4 uTint;\n"
    "void main() {\n"
    "    FragColor = texture(uTexture, vUv) * uTint;\n"
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

}

void Renderer::SetAttributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
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

    if (!spriteShader.LoadFromSource(SPRITE_VS, SPRITE_FS, "sprite_default")) {
        LOG_ERROR("Renderer: 默认 sprite shader 编译失败");
        return false;
    }

    EnsureQuad();

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
    spriteShader.Destroy();
    if (whiteTexture.id_) { glDeleteTextures(1, &whiteTexture.id_); whiteTexture.id_ = 0; }
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

    spriteShader.Use();
    spriteShader.SetInt("uTexture", 0);
    spriteShader.SetVec2("uResolution", (float)width, (float)height);
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

void Renderer::DrawSprite(const Texture* texture, const SDL_Rect& src, const SDL_Rect& dst,
                          SDL_RendererFlip flip, const SDL_Color& tint) {
    if (!ready || !texture || !texture->Valid() || dst.w <= 0 || dst.h <= 0) return;

    SDL_Rect s = src;
    if (s.w <= 0 || s.h <= 0) s = {0, 0, texture->Width(), texture->Height()};
    if (s.w <= 0 || s.h <= 0) return;

    const float texW = (float)texture->Width();
    const float texH = (float)texture->Height();

    spriteShader.SetVec4("uDstRect", (float)dst.x, (float)dst.y, (float)dst.w, (float)dst.h);
    spriteShader.SetVec4("uUvRect", s.x / texW, s.y / texH, s.w / texW, s.h / texH);
    spriteShader.SetVec2("uFlip",
                         (flip & SDL_FLIP_HORIZONTAL) ? 1.0f : 0.0f,
                         (flip & SDL_FLIP_VERTICAL) ? 1.0f : 0.0f);
    spriteShader.SetVec4("uTint", tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id_);
    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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
