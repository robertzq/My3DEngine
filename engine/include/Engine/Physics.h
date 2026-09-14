#pragma once
#include <SDL.h>
#include <vector>
#include "Engine/Entity.h"
#include "Engine/GameObject.h"

class Physics {
public:
    // 基础的 AABB 碰撞检测 (两个矩形是否相交)
    static bool CheckCollision(const SDL_Rect& a, const SDL_Rect& b);

    // 专门处理 GameObject 和 地图障碍物 的碰撞
    // 它会自动修改 obj 的位置 (obj->xpos, obj->ypos) 并处理落地/撞墙
    static void ResolveMapCollision(GameObject* obj, const std::vector<SDL_Rect>& obstacles);

    // 【新增】专门处理 RPG/俯视视角的碰撞
    // 逻辑：如果撞墙，根据重叠深度把玩家“推”出来，保持 X 或 Y 轴不动
    static void ResolveRPGCollision(GameObject* obj, const std::vector<SDL_Rect>& obstacles);

    // 组件化实体的俯视角碰撞：按当前速度把实体推出障碍物
    static void MoveTopDown(Entity& entity, float velX, float velY, const std::vector<SDL_Rect>& obstacles);

    // 组件化实体的横版碰撞：处理落地、顶头、撞墙，并写回 velY / onGround
    static void MovePlatformer(Entity& entity, float velX, float& velY, bool& onGround,
                               const std::vector<SDL_Rect>& obstacles);
};