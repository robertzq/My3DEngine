#include "GameObject.h"
#include "TextureManager.h"

GameObject::GameObject(const char* texturesheet, SDL_Renderer* ren, int x, int y, int numFrames){
    renderer = ren;
    objTexture = TextureManager::LoadTexture(texturesheet, ren);
    
    xpos = x;
    ypos = y;
    
    // 初始化物理参数
    velX = 0;
    velY = 0;
    gravity = 0.5f; // 重力加速度

    // --- 动画初始化 ---
    totalFrames = numFrames;
    animSpeed = 100; // 每100ms切一帧（你可以调整这个数字，越小跑得越快）
    // --- 【核心修改】自动获取图片宽高 ---
    // 参数说明: (纹理, 格式, 访问权限, &宽, &高)
    // 如果图片没加载成功，宽高可能为0，不过后面Update会处理
    // 获取整个大图的宽高
    int texW, texH;
    SDL_QueryTexture(objTexture, NULL, NULL, &texW, &texH);

    // ⚠️ 【关键点 2】确保是 "图片总宽 / 帧数" (texW / totalFrames)
    if (totalFrames > 0) {
        srcRect.w = texW / totalFrames; 
    } else {
        srcRect.w = texW; // 防止除以0
    }
    
    srcRect.h = texH;

    srcRect.x = 0;
    srcRect.y = 0;

    // 打印调试信息
    std::cout << "图片总宽: " << texW << ", 单帧宽: " << srcRect.w << ", 总帧数: " << totalFrames << std::endl;
}

void GameObject::Update() {
    // 1. 物理计算
    velY += gravity;
    xpos += velX;
    ypos += velY;

    // 2. 地面碰撞
    if (ypos >= 500) {
        ypos = 500;
        velY = 0; 
    }

    // 3. 更新渲染位置
    destRect.x = (int)xpos;
    destRect.y = (int)ypos;
  
    // 【建议】手动指定一个合理的大小，比如 64x64 或 128x128
    // 不要直接乘以 2，因为你的新素材非常高清

    float scale = 0.5f; // 缩小到原来的 50% 试试

    destRect.w = srcRect.w * scale;
    destRect.h = srcRect.h * scale;

    if (velX != 0) {
        // === 奔跑状态 ===
        // 原理：SDL_GetTicks() 获取游戏启动后的毫秒数
        // 除以 animSpeed 得到“当前是第几个时间片”
        // 对 totalFrames 取余，保证结果永远在 0 到 totalFrames-1 之间循环
        int currentFrame = (int)((SDL_GetTicks() / animSpeed) % totalFrames);
        
        // 移动切图位置
        srcRect.x = srcRect.w * currentFrame;
    } 
    else {
        // === 静止状态 ===
        // 恢复到第0帧（通常是站立姿势）
        srcRect.x = 0;
    }
}

void GameObject::Render() {
    if(objTexture) {
        TextureManager::Draw(objTexture, srcRect, destRect, renderer, spriteFlip);
    } else {
        // 图片没加载出来，画个红块
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &destRect);
        
        // 【重要】画完红块，把颜色改回白色，不然背景会变红
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
}

// 新增：跳跃功能
void GameObject::Jump() {
    // 直接给向上的速度，不管在哪里
    velY = -10; 
    std::cout << "芜湖！起飞！" << std::endl;
}

// 设置水平速度：正数向右，负数向左，0停止
void GameObject::SetVelX(int velocity) {
    velX = velocity;

    if (velocity > 0) {
        // 向右走：不翻转 (假设素材默认朝右)
        spriteFlip = SDL_FLIP_NONE;
    } 
    else if (velocity < 0) {
        // 向左走：水平翻转
        spriteFlip = SDL_FLIP_HORIZONTAL;
    }
    // 注意：velocity == 0 时不要改变翻转状态，
    // 否则停下来的时候会瞬间变回默认朝向，看起来很怪。
}