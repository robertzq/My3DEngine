#pragma once
#include <SDL.h>
#include <vector>
#include "Engine/Entity.h"

class Physics {
public:
    // 基础的 AABB 碰撞检测 (两个矩形是否相交)
    static bool CheckCollision(const SDL_Rect& a, const SDL_Rect& b);

    // 组件化实体的俯视角碰撞：按当前速度把实体推出障碍物
    static void MoveTopDown(Entity& entity, float velX, float velY, const std::vector<SDL_Rect>& obstacles);

    // 组件化实体的横版碰撞：处理落地、顶头、撞墙，并写回 velY / onGround
    static void MovePlatformer(Entity& entity, float velX, float& velY, bool& onGround,
                               const std::vector<SDL_Rect>& obstacles);
};