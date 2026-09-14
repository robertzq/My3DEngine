#pragma once
#include <SDL.h>
#include <memory>
#include <string>
#include "Engine/Behavior.h"

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
    std::string type;
    std::string tag;

    Transform transform;
    Sprite sprite;
    Collider collider;

    bool visible = true;
    bool alive = true;

    std::unique_ptr<Behavior> behavior;

    SDL_Rect Bounds() const;
    void SetPosition(float x, float y);
    void Render(SDL_Renderer* renderer, const SDL_Rect& camera) const;
};
