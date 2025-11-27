// src/Entities/RPGPlayer.cpp
#include "RPGPlayer.h"
#include <iostream>

RPGPlayer::RPGPlayer(const char* resourceId, SDL_Renderer* ren, int x, int y, int numFrames, int numRows)
    : GameObject(resourceId, ren, x, y, numFrames) // 1. 先调用父类构造
{
    // 2. 初始化子类独有的变量
    totalRows = numRows;
    currentDirection = DOWN;
    isMoving = false;

    // 3. 【修正高度】
    // 父类构造函数里算出的 srcRect.h 是整张图的高度。
    // 在 RPG 模式下，我们要把它除以行数。
    if (totalRows > 0) {
        srcRect.h = srcRect.h / totalRows;
    }

    // 默认关闭重力 (RPG 俯视视角不需要重力)
    SetGravityEnabled(false);
}

void RPGPlayer::Update() {
    // 1. 简单的物理移动 (直接操作 protected 变量)
    xpos += velX;
    ypos += velY;

    // 2. 边界检查 (简单的防止跑出负坐标)
    if (xpos < 0) xpos = 0;
    if (ypos < 0) ypos = 0;

    // 3. 更新渲染目标位置
    destRect.x = static_cast<int>(xpos);
    destRect.y = static_cast<int>(ypos);
    // 保持原来的大小，或者你可以乘 2 放大
    destRect.w = 50;
    destRect.h = 50;

/* // 方法 B：如果你想保留宽高比，只是缩小一点
    // 比如素材很大，你想缩小到 0.3 倍
    float scale = 0.3f;
    destRect.w = static_cast<int>(srcRect.w * scale);
    destRect.h = static_cast<int>(srcRect.h * scale);
    */

    // 4. === RPG 动画状态机 ===
    isMoving = false;

    spriteFlip = SDL_FLIP_NONE;

    if (velY > 0) {
        currentDirection = DOWN;
        isMoving = true;
    }
    else if (velY < 0) {
        currentDirection = UP;
        isMoving = true;
    }

    if (velX > 0) {
        currentDirection = RIGHT;
        isMoving = true;
        // 向右走：不翻转
        spriteFlip = SDL_FLIP_NONE;
    }
    else if (velX < 0) {
        // 【核心修改】向左走时：
        // 1. 仍然使用 RIGHT (第3行) 的图片！
        currentDirection = LEFT;
        // 2. 开启水平翻转，让它看起来像向左
        spriteFlip = SDL_FLIP_NONE;

        isMoving = true;
    }

    // 设置切图的行 (Y轴)
    srcRect.y = currentDirection * srcRect.h;

    // 设置切图的列 (X轴 - 帧动画)
    if (isMoving) {
        int currentFrame = (int)((SDL_GetTicks() / animSpeed) % totalFrames);
        srcRect.x = srcRect.w * currentFrame;
    } else {
        srcRect.x = 0; // 静止帧
    }
}