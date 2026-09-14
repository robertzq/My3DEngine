#include "Engine/TextRenderer.h"
#include "Engine/ResourceManager.h"
#include "Engine/Renderer.h"
#include "Engine/Texture.h"
#include "Engine/Log.h"
#include <vector>

TTF_Font* TextRenderer::font = nullptr;
int TextRenderer::defaultSize = 24;
std::string TextRenderer::fontResourceId;
std::map<int, TTF_Font*> TextRenderer::fonts;
std::map<std::string, Texture*> TextRenderer::cache;

TTF_Font* TextRenderer::LoadFont(int size) {
    const EmbeddedResource* res = ResourceManager::GetResource(fontResourceId);
    if (!res) {
        LOG_ERROR("无法获取字体资源: " << fontResourceId);
        return nullptr;
    }
    SDL_RWops* rw = SDL_RWFromConstMem(res->data, (int)res->size);
    if (!rw) {
        LOG_ERROR("创建字体 RWops 失败: " << SDL_GetError());
        return nullptr;
    }
    TTF_Font* f = TTF_OpenFontRW(rw, 1, size);   // freesrc=1
    if (!f) {
        LOG_ERROR("加载字体失败 (size=" << size << "): " << TTF_GetError());
        return nullptr;
    }
    TTF_SetFontStyle(f, TTF_STYLE_BOLD);
    return f;
}

TTF_Font* TextRenderer::FontFor(int size) {
    if (size <= 0) return font;
    auto it = fonts.find(size);
    if (it != fonts.end()) return it->second;
    TTF_Font* f = LoadFont(size);
    fonts[size] = f;
    return f;
}

bool TextRenderer::Init(const char* fontResourceId_, int fontSize) {
    if (TTF_WasInit() == 0) {
        if (TTF_Init() == -1) {
            LOG_ERROR("TTF_Init Failed: " << TTF_GetError());
            return false;
        }
    }
    fontResourceId = fontResourceId_;
    defaultSize = fontSize > 0 ? fontSize : 24;

    font = LoadFont(defaultSize);
    if (!font) return false;
    fonts[defaultSize] = font;
    LOG_INFO("字体加载成功: " << fontResourceId << " (size=" << defaultSize << ")");
    return true;
}

void TextRenderer::DrawText(int x, int y, std::string text, SDL_Color color, int fontSize) {
    if (text.empty()) return;
    TTF_Font* f = FontFor(fontSize);
    if (!f) return;

    const int size = (fontSize <= 0) ? defaultSize : fontSize;
    std::string key = std::to_string(size) + ":" + std::to_string(color.r) + "," +
                      std::to_string(color.g) + "," + std::to_string(color.b) + ":" + text;

    Texture* texture = nullptr;
    auto it = cache.find(key);
    if (it != cache.end()) {
        texture = it->second;
    } else {
        SDL_Surface* surface = TTF_RenderUTF8_Blended(f, text.c_str(), color);
        if (!surface) return;
        texture = Renderer::CreateTextureFromSurface(surface);
        SDL_FreeSurface(surface);
        if (!texture) return;
        cache[key] = texture;
    }

    SDL_Rect dst = {x, y, texture->Width(), texture->Height()};
    Renderer::DrawSprite(texture, SDL_Rect{0, 0, 0, 0}, dst, SDL_FLIP_NONE, color);
}

SDL_Point TextRenderer::MeasureText(const std::string& text, int fontSize) {
    TTF_Font* f = FontFor(fontSize);
    if (!f || text.empty()) return {0, 0};
    int w = 0, h = 0;
    TTF_SizeUTF8(f, text.c_str(), &w, &h);
    return {w, h};
}

void TextRenderer::DrawTextIn(const SDL_Rect& box, const std::string& text, SDL_Color color,
                              int fontSize, TextAlign hAlign, TextVAlign vAlign) {
    if (text.empty()) return;
    SDL_Point m = MeasureText(text, fontSize);
    int x = box.x;
    if (hAlign == TextAlign::Center) x = box.x + (box.w - m.x) / 2;
    else if (hAlign == TextAlign::Right) x = box.x + box.w - m.x;

    int y = box.y;
    if (vAlign == TextVAlign::Middle) y = box.y + (box.h - m.y) / 2;
    else if (vAlign == TextVAlign::Bottom) y = box.y + box.h - m.y;

    DrawText(x, y, text, color, fontSize);
}

void TextRenderer::Clean() {
    for (auto& pair : cache) Renderer::DestroyTexture(pair.second);
    cache.clear();
    // font 与 fonts[defaultSize] 可能是同一个
    for (auto& pair : fonts) {
        if (pair.second) TTF_CloseFont(pair.second);
    }
    fonts.clear();
    font = nullptr;
}
