#include "Collision.h"

bool Collision::AABB(const SDL_Rect& recA, const SDL_Rect& recB) {
    // 核心逻辑：如果有任何一边错开了，就是没撞上
    if (
        recA.x + recA.w >= recB.x && // A的右边 >= B的左边
        recB.x + recB.w >= recA.x && // B的右边 >= A的左边
        recA.y + recA.h >= recB.y && // A的底边 >= B的顶边
        recB.y + recB.h >= recA.y    // B的底边 >= A的顶边
    ) {
        return true; // 撞上了！
    }
    
    return false; // 没撞上
}