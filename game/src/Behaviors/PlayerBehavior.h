#pragma once
#include <SDL.h>
#include "Engine/Behavior.h"

class PlayerBehavior : public Behavior {
public:
    void OnSpawn(SceneContext& context) override;
    void Update(SceneContext& context, float deltaTime) override;

    void SetInputEnabled(bool enabled) { inputEnabled = enabled; }
    bool IsInputEnabled() const { return inputEnabled; }
    void SetVelocity(float x, float y) { velX = x; velY = y; }

private:
    float velX = 0.0f;
    float velY = 0.0f;
    float speed = 240.0f;     // 像素/秒
    float gravity = 1800.0f;  // 像素/秒²
    bool platformer = false;
    bool onGround = false;
    bool inputEnabled = true;

    int frames = 4;
    int rows = 4;
    int frameW = 0;
    int frameH = 0;
    int direction = 0;
};
