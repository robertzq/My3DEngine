#pragma once
#include "Game.h"

class GameObject {
public:
    GameObject(const char* texturesheet, SDL_Renderer* ren, int x, int y, int numFrames = 1);
   virtual ~GameObject();

    virtual void Update();
    virtual void Render();
    
    // 新增：让外部能控制它跳跃
    void Jump();
    void SetVelX(int velocity); // <--- 新增这一行：设置水平速度
    // 获取当前的碰撞包围盒
    virtual SDL_Rect GetBounds();
    // 强制让角色落地
    void LandOnGround(int groundHeight);

    // 获取当前的速度方向（为了判断是向左撞还是向右撞）
    int GetVelX() { return velX; }
    int GetVelY() { return velY; } // 记得也要加这个，后面会用到

    // 处理撞墙
    void CollideWall(int wallX, int wallWidth);
    // 【新增】开关重力 (默认是 true/开启)
    void SetGravityEnabled(bool enabled) { useGravity = enabled; }

    // 【新增】设置垂直速度 (RPG 模式需要上下走)
    void SetVelY(int velocity);

    void SetX(float x) { xpos = x; }
    void SetY(float y) { ypos = y; }
    // 同时设置 X 和 Y
    void SetPos(float x, float y) { xpos = x; ypos = y; }
protected:
    float xpos, ypos;      // 改成 float 以便计算精细的物理
    float velX, velY;      // 速度 (Velocity)
    float gravity;         // 重力

    // --- 动画相关变量 ---
    int totalFrames;   // 总帧数
    int animSpeed;     // 动画速度（数值越小越快，单位毫秒）
    bool useGravity = true; // 【新增】状态变量
    SDL_Texture* objTexture;
    SDL_Rect srcRect, destRect;
    SDL_Renderer* renderer;
    // 新增：记录当前是否需要翻转
    SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;
};