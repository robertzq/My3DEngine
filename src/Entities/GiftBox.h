#pragma once
#include "Game.h"

class GiftBox {
public:
    // 构造函数：传入位置
    GiftBox(int x, int y);
    ~GiftBox();

    void Render();
    SDL_Rect GetBounds(); // 获取盒子的碰撞框

    void Open();          // 打开盒子的动作
    bool IsOpened();      // 检查盒子是否已经打开
    void closeBanner();
    bool IsBannerClosed();

private:
    SDL_Texture* boxTexture;
    SDL_Texture* bannerTexture;

    SDL_Rect boxRect;    // 盒子的位置和大小
    SDL_Rect bannerRect; // 横幅的位置和大小

    bool isOpened;       // 状态标记：是否打开了

    // --- 【新增】记录打开的时间 ---
    Uint32 openTime = 0;
    bool isBannerDismissed = false;
};