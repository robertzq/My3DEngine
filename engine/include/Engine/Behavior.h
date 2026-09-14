#pragma once
#include <SDL.h>
#include "Engine/json.hpp"

class Entity;
struct SceneContext;

using json = nlohmann::json;

class Behavior {
public:
    virtual ~Behavior() = default;

    Entity* self = nullptr;
    json config;

    virtual void OnSpawn(SceneContext& context) {}
    virtual void Update(SceneContext& context, float deltaTime) {}
    virtual void HandleEvent(SceneContext& context, SDL_Event& event) {}
};
