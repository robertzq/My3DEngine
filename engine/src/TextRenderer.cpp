#include "Engine/TextRenderer.h"
#include "Engine/ResourceManager.h"
#include "Engine/Log.h"
#include <iostream>

// 初始化静态成员
TTF_Font* TextRenderer::font = nullptr;
std::map<std::string, SDL_Texture*> TextRenderer::cache;

bool TextRenderer::Init(const char* fontResourceId, int fontSize) {
    // 1. 初始化 TTF 库
    if (TTF_WasInit() == 0) {
        if (TTF_Init() == -1) {
            LOG_ERROR("TTF_Init Failed: " << TTF_GetError());
            return false;
        }
    }

    // 2. 从 ResourceManager 获取内存数据
    const EmbeddedResource* res = ResourceManager::GetResource(fontResourceId);
    if (!res) {
        LOG_ERROR("无法获取字体资源: " << fontResourceId);
        return false;
    }

    // 3. 创建 SDL_RWops (内存流)
    // res->data 和 res->size 现在可以访问了，因为包含了 EmbeddedAssets.h
    SDL_RWops* rw = SDL_RWFromConstMem(res->data, (int)res->size);
    if (!rw) {
        LOG_ERROR("创建字体 RWops 失败: " << SDL_GetError());
        return false;
    }

    // 4. 加载字体 (参数 1 表示加载后自动释放 rw)
    font = TTF_OpenFontRW(rw, 1, fontSize);
    if (!font) {
        LOG_ERROR("加载字体失败: " << TTF_GetError());
        return false;
    }
    // === 【新增】设置字体为粗体 ===
    TTF_SetFontStyle(font, TTF_STYLE_BOLD);
    LOG_INFO("字体加载成功: " << fontResourceId);
    return true;
}

// 【关键实现】实现 Game.cpp 需要调用的 DrawText
void TextRenderer::DrawText(int x, int y, std::string text, SDL_Color color) {
    if (!font) return;
    if (text.empty()) return;

    // 以「颜色 RGB + 内容」作为缓存 key（alpha 作为运行时变量，不入 key）
    std::string key = std::to_string(color.r) + "," + std::to_string(color.g) + "," +
                      std::to_string(color.b) + ":" + text;

    SDL_Texture* texture = nullptr;
    auto it = cache.find(key);
    if (it != cache.end()) {
        texture = it->second;
    } else {
        // 使用 Blended 抗锯齿渲染
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
        if (!surface) return;
        texture = SDL_CreateTextureFromSurface(Game::renderer, surface);
        SDL_FreeSurface(surface);
        if (!texture) return;
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        cache[key] = texture;
    }

    // 运行时透明度（支持淡入淡出）
    SDL_SetTextureAlphaMod(texture, color.a);

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    SDL_Rect dstRect = { x, y, w, h };
    SDL_RenderCopy(Game::renderer, texture, nullptr, &dstRect);
}

void TextRenderer::Clean() {
    for (auto& pair : cache) {
        SDL_DestroyTexture(pair.second);
    }
    cache.clear();
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
}