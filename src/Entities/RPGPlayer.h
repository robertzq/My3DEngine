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

    // 【新增】专门的方法来开启/关闭移动能力
    void SetInputEnabled(bool enabled);

private:
    int totalRows;
    int currentDirection;
    bool isMoving;

// 【新增】这个变量决定玩家是否能响应按键
    bool inputEnabled = true; // 默认为 true
    // 方向枚举
    enum Direction {
        DOWN = 0,
        UP = 3,
        RIGHT = 2,
        LEFT = 1
    };
};