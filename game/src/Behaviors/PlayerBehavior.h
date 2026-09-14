#pragma once
#include <SDL.h>
#include "Engine/Behavior.h"

class PlayerBehavior : public Behavior {
public:
    void OnSpawn(SceneContext& context) override;
    void Update(SceneContext& context, float deltaTime) override;
    void HandleEvent(SceneContext& context, SDL_Event& event) override;

    void SetInputEnabled(bool enabled) { inputEnabled = enabled; }
    bool IsInputEnabled() const { return inputEnabled; }
    void SetVelocity(float x, float y) { velX = x; velY = y; }

private:
    void Animate(bool moving);

    float velX = 0.0f;
    float velY = 0.0f;
    float speed = 4.0f;
    float gravity = 0.5f;
    bool platformer = false;
    bool onGround = false;
    bool inputEnabled = true;

    int frames = 4;
    int rows = 4;
    int frameW = 0;
    int frameH = 0;
    int direction = 0;
};
