#pragma once
#include <SDL.h>
#include <cstdint>
#include <memory>
#include <string>
#include "Engine/Behavior.h"
#include "Engine/Animator.h"

class Texture;

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    int w = 0;
    int h = 0;
};

struct Sprite {
    Texture* texture = nullptr;   // non-owning（ResourceManager / TextRenderer 拥有）
    SDL_Rect src{0, 0, 0, 0};
    SDL_RendererFlip flip = SDL_FLIP_NONE;
};

// 碰撞层（bit flag）：layer = 自己属于哪层，mask = 自己关心哪些层
namespace Layers {
    enum : uint32_t {
        None       = 0,
        Player     = 1u << 0,
        Enemy      = 1u << 1,
        World      = 1u << 2,
        Projectile = 1u << 3,
        Trigger    = 1u << 4,
        All        = 0xFFFFFFFFu,
    };
}

// 碰撞体：
// * enabled   —— 是否参与任何碰撞
// * isTrigger —— 只做重叠查询，不参与 solid 移动解析
// * layer/mask —— 决定与哪些 Collider 互相作用
// * offset    —— 相对 transform 的碰撞盒（宽/高为 0 时用 transform.w/h）
struct Collider {
    SDL_Rect offset{0, 0, 0, 0};
    bool enabled = false;
    bool isTrigger = false;
    uint32_t layer = Layers::World;
    uint32_t mask = Layers::All;
};

class Entity {
public:
    Entity();
    ~Entity();

    std::string id;
    std::string behaviorName;
    std::string tag;

    Transform transform;
    Sprite sprite;
    Collider collider;
    Animator animator;

    bool visible = true;
    bool alive = true;
    float sortYOffset = 0.0f;

    std::unique_ptr<Behavior> behavior;

    SDL_Rect Bounds() const;
    float SortKey() const { return transform.y + transform.h + sortYOffset; }
    void SetPosition(float x, float y);
    void Render(const SDL_Rect& camera) const;
};
