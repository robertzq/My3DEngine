#include "GiftBox.h"
#include "TextureManager.h"
#include "ResourceManager.h"

GiftBox::GiftBox(int x, int y) {
    // 1. 加载图片素材
    boxTexture = ResourceManager::GetTexture("gift.png");
    bannerTexture = ResourceManager::GetTexture("100daysbanner.png");

// --- 【新增】错误检查 ---
    if (boxTexture == nullptr) {
        std::cout << "ERROR: Gift texture not loaded!" << std::endl;
    }
    if (bannerTexture == nullptr) {
        std::cout << "ERROR: Banner texture not loaded!" << std::endl;
    }
    // 2. 设置礼盒位置和大小 (假设素材是 64x64)
    boxRect.x = x;
    boxRect.y = y;
    boxRect.w = 64;
    boxRect.h = 64;

    // 3. 设置横幅位置和大小 (假设素材是 400x100)
    bannerRect.w = 400;
    bannerRect.h = 100;
    // 让横幅横向居中对齐盒子

// 【关键】确保横幅居中
    bannerRect.x = boxRect.x + (boxRect.w / 2) - (bannerRect.w / 2);

    // 【关键】横幅的高度
    // 把它往上提多一点 (比如 120像素)，防止盖住盒子
    bannerRect.y = boxRect.y - 120;
    // 初始状态是关闭的
    isOpened = false;
}

GiftBox::~GiftBox() {
    SDL_DestroyTexture(boxTexture);
    SDL_DestroyTexture(bannerTexture);
}

SDL_Rect GiftBox::GetBounds() {
    return boxRect;
}

void GiftBox::Open() {
    if (!isOpened) {
        isOpened = true;
        // 这里以后可以加个音效！
    }
}

bool GiftBox::IsOpened() {
    return isOpened;
}

void GiftBox::Render() {
    // 1. 计算屏幕坐标
    SDL_Rect screenBox = boxRect;
    screenBox.x = boxRect.x - Game::camera.x;
    screenBox.y = boxRect.y - Game::camera.y;

    // --- 【核心修改】只有在没打开的时候，才画盒子 ---
    if (!isOpened) {
        if(boxTexture) {

            TextureManager::DrawWhole(boxTexture, screenBox, Game::renderer, SDL_FLIP_NONE);
        }
    }

    // 别忘了把颜色改回白色，否则背景或其他东西会变色
    SDL_SetRenderDrawColor(Game::renderer, 255, 255, 255, 255);
    // -------------------------------------

    // 3. 画横幅 (同理)
    if (isOpened && bannerTexture) {
        SDL_Rect screenBanner = bannerRect;
        screenBanner.x = bannerRect.x - Game::camera.x;
        screenBanner.y = bannerRect.y - Game::camera.y;


        // 画出来 (缩放到 400x100)
        TextureManager::DrawWhole(bannerTexture, screenBanner, Game::renderer, SDL_FLIP_NONE);
    }
}