#include "Engine/Physics.h"
#include <cmath> // abs

bool Physics::CheckCollision(const SDL_Rect& a, const SDL_Rect& b) {
    return SDL_HasIntersection(&a, &b);
}

void Physics::ResolveMapCollision(GameObject* obj, const std::vector<SDL_Rect>& obstacles) {
    SDL_Rect playerRect = obj->GetBounds();

    for (const auto& obstacle : obstacles) {
        SDL_Rect result;
        if (SDL_IntersectRect(&playerRect, &obstacle, &result)) {

            // 判断碰撞深度：宽比较宽，还是高比较高？
            bool isVerticalCollision = result.w > result.h;

            if (isVerticalCollision) {
                // === 垂直碰撞 (踩地 或 顶头) ===

                // 如果不仅是垂直碰撞，而且是向下掉 (velY >= 0)，且玩家主要在方块上方
                if (obj->GetVelY() >= 0 && playerRect.y < obstacle.y) {
                     obj->LandOnGround(obstacle.y);
                }
                // (如果要做顶碎砖块，这里处理 velY < 0)
            }
            else {
                // === 水平碰撞 (撞墙) ===

                // 计算底部的高度差： (玩家脚底) - (方块顶部)
                int footOverlap = (playerRect.y + playerRect.h) - obstacle.y;

                // 容错：如果只是蹭到了地板边缘（比如高度差 < 8），不算撞墙
                if (footOverlap < 8) {
                    // 自动抬腿，或者忽略
                }
                else {
                    // 只有大面积重叠才算真正的撞墙
                    obj->CollideWall(obstacle.x, obstacle.w);
                }
            }

            // 更新一下用于下一次检测的 Rect (因为位置可能变了)
            playerRect = obj->GetBounds();
        }
    }
}

void Physics::ResolveRPGCollision(GameObject* obj, const std::vector<SDL_Rect>& obstacles) {
    SDL_Rect playerRect = obj->GetBounds();

    for (const auto& obstacle : obstacles) {
        if (!SDL_HasIntersection(&playerRect, &obstacle)) continue;

        // 1. 尝试只回退 X 轴
        SDL_Rect xBack = playerRect;
        xBack.x -= (int)obj->GetVelX();
        if (!SDL_HasIntersection(&xBack, &obstacle)) {
            obj->CollideWall(obstacle.x, obstacle.w);
            playerRect = obj->GetBounds();
            continue;
        }

        // 2. 尝试只回退 Y 轴
        SDL_Rect yBack = playerRect;
        yBack.y -= (int)obj->GetVelY();
        if (!SDL_HasIntersection(&yBack, &obstacle)) {
            if (obj->GetVelY() > 0) {
                 obj->LandOnGround(obstacle.y);
            } else if (obj->GetVelY() < 0) {
                 obj->LandOnGround(obstacle.y + obstacle.h + playerRect.h);
            }
            playerRect = obj->GetBounds();
        }
    }
}