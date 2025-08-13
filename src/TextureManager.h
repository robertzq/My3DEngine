#pragma once
#include "Game.h" // 前向声明或包含SDL头文件

class TextureManager {
public:
    static SDL_Texture* LoadTexture(const char* fileName, SDL_Renderer* ren);
    static void Draw(SDL_Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_Renderer* ren,  SDL_RendererFlip flip);
};