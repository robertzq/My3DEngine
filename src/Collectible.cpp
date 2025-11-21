#include "Collectible.h"
#include "TextureManager.h"
#include <cmath> // 用来算 sin

Collectible::Collectible(int x, int y) {
    texture = TextureManager::LoadTexture("./assets/heart.png", Game::renderer);

    rect.x = x;
    rect.y = y;
    rect.w = 40;
    rect.h = 32;
    startY = y;
    floatAngle = 0.0f;
    collected = false;
}

Collectible::~Collectible() {
    SDL_DestroyTexture(texture);
}

void Collectible::Update() {
    // 让爱心上下浮动，看起来更诱人
    floatAngle += 0.1f;
    rect.y = startY + sin(floatAngle) * 5; // 上下浮动 5 像素
}

void Collectible::Render() {
    // 1. 如果被收集了，直接返回，不画
    if (collected) return;

    // 2. 计算屏幕坐标
    SDL_Rect screenRect = rect;
    screenRect.x = rect.x - Game::camera.x; // 减去摄像机
    screenRect.y = rect.y - Game::camera.y;

    // 3. 画图
    if (texture) {

        TextureManager::DrawWhole(texture, screenRect, Game::renderer, SDL_FLIP_NONE);
    }
}

SDL_Rect Collectible::GetBounds() { return rect; }