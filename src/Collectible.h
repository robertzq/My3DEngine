#pragma once
#include "Game.h"

class Collectible {
public:
    Collectible(int x, int y);
    ~Collectible();

    void Update(); // 比如可以让爱心上下浮动一下，更生动
    void Render();
    SDL_Rect GetBounds();

    bool IsCollected() { return collected; }
    void Collect() { collected = true; }

private:
    SDL_Texture* texture;
    SDL_Rect rect;
    bool collected;

    // 浮动动画用
    int startY;
    float floatAngle;
};