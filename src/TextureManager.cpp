// TextureManager.cpp
#include "TextureManager.h"
#include <SDL_image.h> // 确认头文件

SDL_Texture* TextureManager::LoadTexture(const char* texture, SDL_Renderer* ren) {
    // 1. 加载图片到 Surface
    SDL_Surface* tempSurface = IMG_Load(texture);
    
    // --- 【新增】错误打印 ---
    if (tempSurface == nullptr) {
        std::cout << "!!! 图片加载失败 !!!" << std::endl;
        std::cout << "路径: " << texture << std::endl;
        std::cout << "原因: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    // ----------------------

    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, tempSurface);
    SDL_FreeSurface(tempSurface);

    return tex;
}

void TextureManager::Draw(SDL_Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_Renderer* ren, SDL_RendererFlip flip) {

    if (dest.w == 0 || dest.h == 0) {
        std::cout << "WARNING: Destination width or height is 0!" << std::endl;
    }

    SDL_RenderCopyEx(ren, tex, &src, &dest, 0.0, NULL, flip);
}