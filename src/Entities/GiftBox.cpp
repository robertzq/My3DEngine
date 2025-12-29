#include "GiftBox.h"
#include "TextureManager.h"
#include "ResourceManager.h"
#include "TextRenderer.h"

GiftBox::GiftBox(int x, int y) {
    // 1. 加载图片素材
    boxTexture = ResourceManager::GetTexture("gift.png");
    bannerTexture = ResourceManager::GetTexture("birthbanner.png");
    isBannerDismissed = false;
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
    bannerRect.x = boxRect.x + (boxRect.w / 2) - (bannerRect.w / 2);
    bannerRect.y = boxRect.y - 120; // 往上提，给下面的字留点空间
    // 让横幅横向居中对齐盒子

// 【关键】确保横幅居中
    bannerRect.x = boxRect.x + (boxRect.w / 2) - (bannerRect.w / 2);

    // 【关键】横幅的高度
    // 把它往上提多一点 (比如 120像素)，防止盖住盒子
    bannerRect.y = boxRect.y - 120;
    // 初始状态是关闭的
    isOpened = false;
    openTime = 0; // 初始化
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
        openTime = SDL_GetTicks(); // 【关键】记录打开的瞬间时间
        std::cout << "礼物盒打开！时间戳：" << openTime << std::endl;
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
    //if (!isOpened) { //注释掉以后，弹出banner后，不会让礼盒消失
    if(boxTexture) {

        TextureManager::DrawWhole(boxTexture, screenBox, Game::renderer, SDL_FLIP_NONE);
    }
    //}

    // 别忘了把颜色改回白色，否则背景或其他东西会变色
    SDL_SetRenderDrawColor(Game::renderer, 255, 255, 255, 255);
    // -------------------------------------

    // 3. 画横幅 (同理)
    if (isOpened && bannerTexture && !isBannerDismissed) {
        SDL_Rect screenBanner = bannerRect;
        screenBanner.x = bannerRect.x - Game::camera.x;
        screenBanner.y = bannerRect.y - Game::camera.y;
        // 画出来 (缩放到 400x100)
        TextureManager::DrawWhole(bannerTexture, screenBanner, Game::renderer, SDL_FLIP_NONE);

        // --- 【新增】检测时间，延迟显示提示语 ---
        Uint32 currentTime = SDL_GetTicks();

        // 如果已经打开超过 2000 毫秒 (2秒)
        if (currentTime - openTime > 2000) {

            // 提示语内容
            std::string tip = "按回车继续旅行，去爱心湖那里看看吧";

            // 计算文字位置：放在横幅正下方
            // 假设文字大概 16px 大小，这一行字大概宽 300px 左右
            // screenBanner.x 是横幅左边，横幅宽400。中心是 screenBanner.x + 200
            // 我们简单居中一下
            int textX = screenBanner.x + 60;
            int textY = screenBanner.y + 110; // 横幅高100，所以在横幅下面10px

            // 为了让字更明显，我们可以加个简单的闪烁效果 (Blinking)
            // 每秒闪烁一次
            SDL_Color textColor = {255, 255, 255, 255}; // 白色
            if ((currentTime / 500) % 2 == 0) {
                textColor = {255, 215, 0, 255}; // 金色
            }

            // 调用你的 TextRenderer 画字
            // 参数：内容, x, y, 字号, 颜色
            TextRenderer::DrawText(textX, textY, tip, textColor);
        }
    }
    // 恢复绘制颜色，是个好习惯
    SDL_SetRenderDrawColor(Game::renderer, 255, 255, 255, 255);

}

void GiftBox::closeBanner() {
    isBannerDismissed = true;
}

bool GiftBox::IsBannerClosed() {
    return isBannerDismissed;
}