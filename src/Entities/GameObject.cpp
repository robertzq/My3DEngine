#include "GameObject.h"
#include "TextureManager.h"
#include "ResourceManager.h"

GameObject::GameObject(const char* resourceId, SDL_Renderer* ren, int x, int y, int numFrames){
    renderer = ren;
    objTexture = ResourceManager::GetTexture(resourceId);
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
    if (objTexture) {
            SDL_QueryTexture(objTexture, NULL, NULL, &texW, &texH);
    } else {
        std::cout << "ERROR: GameObject 纹理加载失败 -> " << resourceId << std::endl;
    }

    // 【关键点 2】确保是 "图片总宽 / 帧数" (texW / totalFrames)
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
    if (useGravity) {
            velY += gravity;
    }
    xpos += velX;
    ypos += velY;

    // --- 【新增】世界边界限制 ---

    // 1. 左边界限制
    if (xpos < 0) {
        xpos = 0;
    }

    // 2. 右边界限制
    // 假设地图宽 100 格 * 32 = 3200
    // srcRect.w 是小人的宽度（或者用 destRect.w 如果你缩放过）
    // 这里的 3200 最好不要写死，以后可以传进来，但现在先写死测试
    if (xpos > 3200 - destRect.w) {
        xpos = 3200 - destRect.w;
    }

    // 3. 更新渲染位置
    destRect.x = static_cast<int>(xpos);
    destRect.y = static_cast<int>(ypos);
  
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
        // 1. 创建一个临时的矩形，专门用来画在屏幕上
        SDL_Rect screenRect = destRect;

        // 2. 【核心修正】必须减去摄像机的位置！
        // 这样就把 "世界坐标" 转换成了 "屏幕坐标"
        screenRect.x = destRect.x - Game::camera.x;
        screenRect.y = destRect.y - Game::camera.y;

        // 3. 画这个转换后的 screenRect，而不是原始的 destRect
        TextureManager::Draw(objTexture, srcRect, screenRect, renderer, spriteFlip);
    } else {

    // 【修复】这里原来直接画了 destRect，导致红块不随摄像机移动
            SDL_Rect screenRect = destRect;
            screenRect.x = destRect.x - Game::camera.x; // <--- 加上这一行
            screenRect.y = destRect.y - Game::camera.y; // <--- 加上这一行

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

void GameObject::SetVelY(int velocity) {
    velY = velocity;
    // RPG 模式一般不需要垂直翻转图片，除非你有“背影”素材
}

void GameObject::LandOnGround(int groundHeight) {
   // 1. 修正物理位置
       ypos = groundHeight - destRect.h;
       velY = 0;

       // 2. 【核心修复】立刻同步视觉位置！
       // 如果不加这行，这一帧渲染的还是那个“掉进地里 0.5 像素”的旧 destRect
       // 加了这行，渲染器就会立刻画在修正后的正确位置
       destRect.y = static_cast<int>(ypos);
}

// 处理撞墙逻辑
void GameObject::CollideWall(int wallX, int wallWidth) {
    // 如果我正向右走 (velX > 0)，撞到了方块的左边
    if (velX > 0) {
        xpos = wallX - destRect.w; // 停在方块左边
    }
    // 如果我正向左走 (velX < 0)，撞到了方块的右边
    else if (velX < 0) {
        xpos = wallX + wallWidth; // 停在方块右边
    }
    
    // 这里的关键是：我们只改变了位置，不一定非要 velX = 0。
    // 如果你希望像超级马里奥那样按住方向键继续跑但不移动，就不设 velX=0。
    // 如果你希望撞墙瞬间停下，可以加一句 velX = 0;
    
    destRect.x = (int)xpos; // 更新渲染位置
}

SDL_Rect GameObject::GetBounds() {
    SDL_Rect bounds = destRect;

    // 1. 左右缩进：改回合理的数值
    // 12 太大了，会导致你离墙很远。6 比较合适 (左右各缩6，总共缩12)
    int bufferX = 6;

    // 2. 头部缩进：防止跳跃时头皮蹭到天花板
    int bufferY_Top = 4;

    // 3. 【核心修复】脚底不缩进！
    // 这样物理落地时，视觉上也刚好落地
    int bufferY_Bottom = 0;

    bounds.x = destRect.x + bufferX;
    bounds.w = destRect.w - (2 * bufferX); // 宽度减小

    bounds.y = destRect.y + bufferY_Top; // 顶部下移

    // 高度 = 原高度 - 顶部缩进 - 底部缩进(0)
    bounds.h = destRect.h - bufferY_Top - bufferY_Bottom;

    return bounds;
}
// 【新增】析构函数实现
GameObject::~GameObject() {
    // 这里并没有什么特别需要手动释放的
    // 因为 TextureManager 会处理渲染器，但为了代码完整性，这里得有个空函数
    std::cout << "GameObject 已销毁" << std::endl;
}