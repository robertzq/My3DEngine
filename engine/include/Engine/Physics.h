#pragma once
#include <SDL.h>
#include <vector>
#include "Engine/Entity.h"

class Physics {
public:
    // --- Collision Query（只查询，不改动任何东西）---

    // 基础 AABB 相交
    static bool CheckCollision(const SDL_Rect& a, const SDL_Rect& b);

    // 两个 Collider 是否允许互相作用（layer / mask 过滤）
    static bool ShouldCollide(const Collider& a, const Collider& b);

    // 实体之间的重叠查询：双方 enabled + layer/mask 匹配 + AABB 相交
    static bool Overlap(const Entity& a, const Entity& b);

    // 实体与任意矩形（瓦片触发器 / 区域）的重叠查询：只要求实体 collider enabled
    static bool Overlap(const Entity& entity, const SDL_Rect& rect);

    // --- Collision Resolution（只处理世界 solid 矩形，即 TileMap::Colliders()）---
    // stepX / stepY 为本帧位移（像素），不是速度；落地/撞墙时直接修正 entity.transform。

    // 俯视角：把实体沿阻塞轴推出 solid 矩形
    static void MoveTopDown(Entity& entity, float stepX, float stepY,
                            const std::vector<SDL_Rect>& obstacles);

    // 横版：处理落地 / 顶头 / 撞墙，并写回 stepY（命中置 0）与 onGround
    static void MovePlatformer(Entity& entity, float stepX, float& stepY, bool& onGround,
                               const std::vector<SDL_Rect>& obstacles);
};
