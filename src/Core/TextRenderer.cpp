#include "TextRenderer.h"
#include "ResourceManager.h"
#include "EmbeddedAssets.h"  // 【关键修复】必须包含这个文件，否则不认识 EmbeddedResource
#include <iostream>

// 初始化静态成员
// 注意：不要再定义 fontTexture 或 labels 了，因为 TextRenderer.h 里没有它们
TTF_Font* TextRenderer::font = nullptr;

bool TextRenderer::Init(const char* fontResourceId, int fontSize) {
    // 1. 初始化 TTF 库
    if (TTF_WasInit() == 0) {
        if (TTF_Init() == -1) {
            std::cout << "TTF_Init Failed: " << TTF_GetError() << std::endl;
            return false;
        }
    }

    // 2. 从 ResourceManager 获取内存数据
    const EmbeddedResource* res = ResourceManager::GetResource(fontResourceId);
    if (!res) {
        std::cout << "无法获取字体资源: " << fontResourceId << std::endl;
        return false;
    }

    // 3. 创建 SDL_RWops (内存流)
    // res->data 和 res->size 现在可以访问了，因为包含了 EmbeddedAssets.h
    SDL_RWops* rw = SDL_RWFromConstMem(res->data, (int)res->size);
    if (!rw) {
        std::cout << "创建字体 RWops 失败: " << SDL_GetError() << std::endl;
        return false;
    }

    // 4. 加载字体 (参数 1 表示加载后自动释放 rw)
    font = TTF_OpenFontRW(rw, 1, fontSize);
    if (!font) {
        std::cout << "加载字体失败: " << TTF_GetError() << std::endl;
        return false;
    }
    // === 【新增】设置字体为粗体 ===
    TTF_SetFontStyle(font, TTF_STYLE_BOLD);
    std::cout << "字体加载成功: " << fontResourceId << std::endl;
    return true;
}

// 【关键实现】实现 Game.cpp 需要调用的 DrawText
void TextRenderer::DrawText(int x, int y, std::string text, SDL_Color color) {
    if (!font) return;
    if (text.empty()) return;

    // 1. 将文字渲染为 Surface
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
    if (!surface) {
        return;
    }

    // 2. 将 Surface 转为 Texture
    SDL_Texture* texture = SDL_CreateTextureFromSurface(Game::renderer, surface);

    // 3. 设置绘制位置和大小
    SDL_Rect dstRect = { x, y, surface->w, surface->h };

    // 4. 绘制
    SDL_RenderCopy(Game::renderer, texture, nullptr, &dstRect);

    // 5. 立即清理内存（TTF渲染的纹理是一次性的）
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void TextRenderer::Clean() {
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
}