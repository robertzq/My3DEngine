#include "PlayScene.h"
#include "../Core/Game.h"
#include "Physics.h"
#include <iostream>
#include "SceneFactory.h"

PlayScene::PlayScene(std::string mapFile) : mapPath(mapFile) {}

void PlayScene::OnEnter() {
    std::cout << "Enter PlayScene (Map: " << mapPath << ")..." << std::endl;

    // 1. 创建地图
    map = new Map();
    map->LoadMap(mapPath);

    // 2. 创建玩家
    player = new GameObject("redPlayer.png", Game::renderer, 0, 0, 6);

    // 3. 创建礼盒
    giftBox = new GiftBox(2850, 200);

    // 4. 生成爱心
    hearts = map->generateHearts();

    // 1. 计算玩家当前的理想摄像机位置
    float startCameraX = player->GetBounds().x - (800 / 2);
    if (startCameraX < 0) startCameraX = 0;
    if (startCameraX > 3200 - 800) startCameraX = 3200 - 800;

    // 2. 强行覆盖全局静态变量，消除“记忆”
    Game::cameraX_float = startCameraX;
    Game::camera.x = static_cast<int>(startCameraX);
    Game::camera.y = 0;
}

void PlayScene::OnExit() {
    std::cout << "离开 PlayScene, 清理内存..." << std::endl;
    delete player;
    delete map;
    delete giftBox;
    for (auto heart : hearts) delete heart;
    hearts.clear();
}

void PlayScene::HandleEvents(SDL_Event& event) {
    // 处理玩家控制
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_SPACE: player->Jump(); break;
            case SDLK_a: player->SetVelX(-5); break;
            case SDLK_d: player->SetVelX(5); break;
        }
    }
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_a:
            case SDLK_d: player->SetVelX(0); break;
        }
    }
}

void PlayScene::Update() {
    // 1. 更新对象状态
    player->Update();
std::cout << "PlayScene Camera 地址: " << &Game::camera << " 值: " << Game::camera.x << std::endl;
    // 2. 物理碰撞：玩家 vs 地图 (使用新封装的 Physics 类)
    std::vector<SDL_Rect> obstacles = map->getColliders();
    Physics::ResolveMapCollision(player, obstacles);

    // 3. 摄像机跟随 (逻辑保持不变)
    float targetCameraX = player->GetBounds().x - (800 / 2);
    if (targetCameraX < 0) targetCameraX = 0;
    if (targetCameraX > 3200 - 800) targetCameraX = 3200 - 800;

    Game::cameraX_float += (targetCameraX - Game::cameraX_float) * 0.1f;
    Game::camera.x = static_cast<int>(Game::cameraX_float);
    Game::camera.y = 0;

    // 4. 礼盒交互
    if (!giftBox->IsOpened()) {
        SDL_Rect pRect = player->GetBounds();
        SDL_Rect bRect = giftBox->GetBounds();

        // 简单检测：只有当玩家向上顶 (velY < 0) 且在盒子下方时才触发
        if (Physics::CheckCollision(pRect, bRect)) {
            if (player->GetVelY() < 0 && pRect.y > bRect.y) {
                giftBox->Open();
                player->LandOnGround(bRect.y); // 顶到东西反弹/停住
            }
        }
    }

    // 5. 爱心收集
    for (auto heart : hearts) {
        if (!heart->IsCollected()) {
            heart->Update();
            if (Physics::CheckCollision(player->GetBounds(), heart->GetBounds())) {
                heart->Collect();
            }
        }
    }
}

void PlayScene::Render() {
    // 背景/地图
    map->DrawMap();
    // 玩家
    player->Render();
    // 礼盒
    giftBox->Render();
    // 爱心
    for (auto heart : hearts) heart->Render();
}

// 【新增】这一行代码会让 PlayScene 自动注册自己！
// 不需要 Game.cpp 知道 PlayScene 的存在
static SceneFactory::Proxy proxy("PlayScene", [](const json& params) {
    std::string mapPath = params.value("map_file", "level1.map");
    return new PlayScene(mapPath);
});