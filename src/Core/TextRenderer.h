#pragma once
#include "Game.h"
#include <string>
#include <map>
#include <SDL_ttf.h>

class TextRenderer {
public:
    static bool Init(const char* fontResourceId, int fontSize);

    static void Clean();
    // 渲染文字函数
    static void DrawText(int x, int y, std::string text, SDL_Color color = {255, 255, 255, 255});

private:
    static TTF_Font* font;
};