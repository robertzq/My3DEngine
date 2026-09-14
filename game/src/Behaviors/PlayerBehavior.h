#pragma once
#include <SDL.h>
#include "Engine/Behavior.h"

class PlayerBehavior : public Behavior {
public:
    void OnSpawn(SceneContext& context) override;
    void Update(SceneContext& context, float deltaTime) override;

private:
    float speed = 180.0f;   // 像素/秒
    float velX = 0.0f;
    float velY = 0.0f;

    int cols = 6;
    int rows = 5;
    int frameW = 0;
    int frameH = 0;
};
