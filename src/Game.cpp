#include "Game.h"
#include "TextureManager.h"
#include "GameObject.h"  // 必须包含这个，因为我们要用 player 的 new 方法
#include "Map.h"
#include "GiftBox.h"
#include "Collectible.h"
#include <vector>

// 【修复1】静态变量必须在 cpp 文件里初始化！
SDL_Renderer* Game::renderer = nullptr;
// 1. 在文件最上面初始化静态变量
SDL_Rect Game::camera = {0, 0, 800, 640}; // 假设你的窗口是 800x640
float Game::cameraX_float = 0.0f;

Game::Game() {}
Game::~Game() {}

// Game.cpp

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
    int flags = 0;
    if (fullscreen) {
        flags = SDL_WINDOW_FULLSCREEN;
    }

    // 1. 初始化 SDL 基础功能
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
        std::cout << "SDL 初始化成功..." << std::endl;

        // --- 【新增】2. 初始化 PNG 图片加载模块 ---
        // 这一步非常关键！没有它，LoadTexture 读 PNG 会直接失败
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            std::cout << "SDL_image 初始化失败: " << IMG_GetError() << std::endl;
            isRunning = false;
            return;
        }
        // ---------------------------------------

        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        
        if (renderer) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            isRunning = true;
        }
    } else {
        isRunning = false;
    }

    // 创建地图对象
    map = new Map();

    // 加载这个设计好的关卡
    map->LoadMap("./assets/level1.map");
    // 创建对象
    player = new GameObject("./assets/redPlayer.png", renderer, 0, 0,6);
    // 【新增】创建礼盒
        // 假设地图宽 3200，我们放在快到终点的位置 (比如 x=3000)
        // 放在高空 (比如 y=200)，这样需要跳起来才够得着
    giftBox = new GiftBox(2850, 200);

    hearts = map->generateHearts();

}

// 在 Game.cpp 的 handleEvents 函数里
void Game::handleEvents() {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_QUIT:
            isRunning = false;
            break;

        // --- 按下按键：给速度 ---
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_SPACE:
                    player->Jump();
                    break;
                case SDLK_a: // 按 A，向左走
                    player->SetVelX(-5); 
                    break;
                case SDLK_d: // 按 D，向右走
                    player->SetVelX(5);
                    break;
                default:
                    break;
            }
            break;

        // --- 松开按键：速度归零 ---
        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
                case SDLK_a: 
                    // 松开 A，停下
                    player->SetVelX(0);
                    break;
                case SDLK_d:
                    // 松开 D，停下
                    player->SetVelX(0);
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}

void Game::update() {
    // 【关键修复点】调用 player 的更新
    player->Update();

    // 获取玩家当前的矩形
    SDL_Rect playerRect = player->GetBounds();
    
    // 1. 从地图获取所有障碍物
    std::vector<SDL_Rect> obstacles = map->getColliders();

    // 2. 遍历检查每一个障碍物
    // (这种写法叫 range-based for loop，C++11 特性)
    for (const auto& obstacle : obstacles) {
        
        // --- 【核心修改】使用 SDL_IntersectRect ---
        // 它不仅判断是否相交，还会把相交的区域存入 result
        SDL_Rect result;
        if (SDL_IntersectRect(&playerRect, &obstacle, &result)) {
            
            // 判断碰撞深度：宽比较宽，还是高比较高？
            bool isVerticalCollision = result.w > result.h;

            if (isVerticalCollision) {
                // === 垂直碰撞 (踩地 或 顶头) ===
                
                // 如果我们原本是向下掉的 (velY >= 0) 且 玩家主要在方块上方
                // 这里加个简单判定：碰撞区域在玩家底部
                if (player->GetVelY() >= 0) {
                     player->LandOnGround(obstacle.y);
                }
                // 如果你是要做“顶碎砖块”，在这里加 else 处理 velY < 0 的情况
            } 
            else {
                // 【新增】台阶容错判断
                    // 如果这个“墙”的高度非常低（比如只是脚底蹭到了下面方块的顶部 4 像素以内）
                    // 就不算撞墙，而是让物理引擎把它自动“抬”上去

                    // 计算底部的高度差： (玩家脚底) - (方块顶部)
                    int footOverlap = (playerRect.y + playerRect.h) - obstacle.y;

                    // 如果重叠高度很小（比如小于 8 像素），说明只是蹭到了地板边缘，不是撞墙
                    if (footOverlap < 8) {
                        // 不要推回去，忽略这次水平碰撞
                        // 或者甚至可以帮玩家抬高一点： player->ypos -= footOverlap;
                    }
                    else {
                        // 只有大面积重叠才算真正的撞墙
                        player->CollideWall(obstacle.x, obstacle.w);
                    }
            }
        }
    }

    // 3. 【核心修正】所有位置都确定了，最后才移动摄像机跟随

       // 目标位置：玩家想让摄像机去哪里？(屏幕中心)
           float targetCameraX = player->GetBounds().x - (800 / 2);

           // 边界锁定 (目标位置不能超出地图)
           if (targetCameraX < 0) targetCameraX = 0;
           if (targetCameraX > 3200 - 800) targetCameraX = 3200 - 800;

           // --- 【关键算法】平滑移动 (Lerp) ---
           // 0.1f 是平滑系数：
           // - 1.0f = 瞬间对齐 (和你原来一样，会抖)
           // - 0.05f = 超级慢，像电影镜头
           // - 0.1f ~ 0.2f = 最佳手感
           Game::cameraX_float += (targetCameraX - Game::cameraX_float) * 0.1f;

           // 最后把 float 转回 int 给 SDL 使用
           Game::camera.x = static_cast<int>(Game::cameraX_float);

           // Y轴保持锁定
           camera.y = 0;

     // 只有当盒子还没打开时才检测
         if (!giftBox->IsOpened()) {
             SDL_Rect playerRect = player->GetBounds();
             SDL_Rect boxRect = giftBox->GetBounds();
             SDL_Rect result;

             // 1. 检测是否相交
             if (SDL_IntersectRect(&playerRect, &boxRect, &result)) {
                 // 2. 核心判断条件：
                 // 条件A: 玩家正在向上移动 (velY < 0)
                 // 条件B: 碰撞区域是扁平的 (result.w > result.h)，说明是上下碰撞
                 // 条件C: 玩家在盒子下方 (playerRect.y > boxRect.y) - 确保是头顶撞的

                 if (playerRect.y > boxRect.y) {
                     // 触发！
                     giftBox->Open();

                     // 可选：让玩家撞到后立刻开始下落，更有“顶到东西”的感觉
                     // 这需要 GameObject 提供一个 SetVelY(0) 的方法
                     player->LandOnGround(boxRect.y);
                 }
             }
         }

             for (auto heart : hearts) {
                 if (!heart->IsCollected()) {
                     heart->Update();

                     // 检测碰撞
                     SDL_Rect pRect = player->GetBounds();
                     SDL_Rect hRect = heart->GetBounds();
                     if (Collision::AABB(pRect, hRect)) {
                         heart->Collect();
                         // 这里以后可以加分或者播放音效
                     }
                 }
             }
}

void Game::render() {
    SDL_RenderClear(renderer);
    map->DrawMap();
    // 【关键修复点】调用 player 的渲染
    player->Render();
    // 【新增】画礼盒 (在玩家之前或之后画都行)
    giftBox->Render();

   for (auto heart : hearts) {
       // 临时打印一下，看看是不是在画
       // std::cout << "Drawing heart..." << std::endl;
       heart->Render();
   }
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    delete giftBox; // 【新增】别忘了清理
    delete map; // 别忘了删地图
    delete player;

    // 清理爱心
    for (auto heart : hearts) {
        delete heart;
    }
    hearts.clear();
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    std::cout << "游戏清理完成" << std::endl;
}