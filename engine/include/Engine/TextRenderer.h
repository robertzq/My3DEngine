#pragma once
#include <string>
#include <map>
#include <SDL_ttf.h>

class Texture;

enum class TextAlign { Left, Center, Right };
enum class TextVAlign { Top, Middle, Bottom };

class TextRenderer {
public:
    static bool Init(const char* fontResourceId, int fontSize);

    static void Clean();

    // 在 (x,y)（左上角）绘制；fontSize<=0 用默认字号
    static void DrawText(int x, int y, std::string text, SDL_Color color = {255, 255, 255, 255},
                         int fontSize = 0);

    // 在给定 box 内按对齐方式绘制（UI 用）
    static void DrawTextIn(const SDL_Rect& box, const std::string& text, SDL_Color color,
                           int fontSize, TextAlign hAlign, TextVAlign vAlign);

    // 测量文本像素尺寸（fontSize<=0 用默认字号）
    static SDL_Point MeasureText(const std::string& text, int fontSize = 0);

private:
    static TTF_Font* FontFor(int fontSize);
    static TTF_Font* LoadFont(int fontSize);

    static TTF_Font* font;                 // 默认字号字体
    static int defaultSize;
    static std::string fontResourceId;
    static std::map<int, TTF_Font*> fonts; // size -> font
    // 文字纹理缓存：key = "size:r,g,b:text"
    static std::map<std::string, Texture*> cache;
};
