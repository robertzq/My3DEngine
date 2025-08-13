#pragma once
#include "Game.h"

class GameObject {
public:
    GameObject(const char* texturesheet, SDL_Renderer* ren, int x, int y, int numFrames = 1);
    ~GameObject();

    void Update();
    void Render();
    
    // 新增：让外部能控制它跳跃
    void Jump();
    void SetVelX(int velocity); // <--- 新增这一行：设置水平速度

private:
    float xpos, ypos;      // 改成 float 以便计算精细的物理
    float velX, velY;      // 速度 (Velocity)
    float gravity;         // 重力

    // --- 动画相关变量 ---
    int totalFrames;   // 总帧数
    int animSpeed;     // 动画速度（数值越小越快，单位毫秒）

    SDL_Texture* objTexture;
    SDL_Rect srcRect, destRect;
    SDL_Renderer* renderer;
    // 新增：记录当前是否需要翻转
    SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;
};