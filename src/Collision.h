#pragma once
#include <SDL.h>

class Collision {
public:
    // 传入两个矩形，返回是否重叠 (true/false)
    static bool AABB(const SDL_Rect& recA, const SDL_Rect& recB);
};