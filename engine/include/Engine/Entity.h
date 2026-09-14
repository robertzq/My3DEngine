#pragma once
#include <SDL.h>
#include <memory>
#include <string>
#include "Engine/Behavior.h"
#include "Engine/Animator.h"

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    int w = 0;
    int h = 0;
};

struct Sprite {
    SDL_Texture* texture = nullptr;
    SDL_Rect src{0, 0, 0, 0};
    SDL_RendererFlip flip = SDL_FLIP_NONE;
};

struct Collider {
    SDL_Rect offset{0, 0, 0, 0};
    bool enabled = false;
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
    void Render(SDL_Renderer* renderer, const SDL_Rect& camera) const;
};
