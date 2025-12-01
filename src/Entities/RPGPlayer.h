// src/Entities/RPGPlayer.h
#pragma once
#include "GameObject.h"

class RPGPlayer : public GameObject {
public:
    // 构造函数多了一个 numRows 参数
    RPGPlayer(const char* resourceId, SDL_Renderer* ren, int x, int y, int numFrames, int numRows);

    // 重写 Update，实现 4 方向动画逻辑
    void Update() override;

    SDL_Rect GetBounds() override;
private:
    int totalRows;
    int currentDirection;
    bool isMoving;

    // 方向枚举
    enum Direction {
        DOWN = 0,
        UP = 3,
        RIGHT = 2,
        LEFT = 1
    };
};