#include "Collectible.h"
#include "../Core/TextureManager.h"  // <--- 改成相对路径
#include "../Core/ResourceManager.h" // <--- 改成相对路径
#include "../Core/Game.h"            // <--- 强制引用 Core 里的 Game.h
#include <cmath> // 用来算 sin

Collectible::Collectible(int x, int y) {
    texture = ResourceManager::GetTexture("heart.png");

    rect.x = x;
    rect.y = y;
    rect.w = 40;
    rect.h = 32;
    startY = y;
    floatAngle = 0.0f;
    collected = false;
}

Collectible::~Collectible() {

}

void Collectible::Update() {
    // 让爱心上下浮动，看起来更诱人
    floatAngle += 0.1f;
    //rect.y = startY + sin(floatAngle) * 5; // 上下浮动 5 像素
}

void Collectible::Render() {
    // 1. 如果被收集了，直接返回，不画
    if (collected) return;
std::cout << "Collectible Camera 地址: " << &Game::camera << " 值: " << Game::camera.x << std::endl;
    // 2. 计算屏幕坐标
    SDL_Rect screenRect = rect;
    screenRect.x = rect.x - Game::camera.x; // 减去摄像机
    screenRect.y = rect.y - Game::camera.y + (int)(sin(floatAngle) * 5);

// 调试打印 (如果你发现还不对，把这行注释打开，看看控制台输出什么)

    if (screenRect.x > 0 && screenRect.x < 800) {
        std::cout << "爱心 ScreenX: " << screenRect.x
                  << " (World: " << rect.x << " - Cam: " << Game::camera.x << ")" << std::endl;
    }

    // 3. 画图
    if (texture) {

        TextureManager::DrawWhole(texture, screenRect, Game::renderer, SDL_FLIP_NONE);
    }
}

SDL_Rect Collectible::GetBounds() { return rect; }