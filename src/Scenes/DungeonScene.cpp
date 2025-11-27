#include "DungeonScene.h"
#include "Game.h"
#include "Physics.h"
#include "ResourceManager.h"
#include "SceneFactory.h"
#include "RPGPlayer.h"

DungeonScene::DungeonScene(std::string mapFile) : mapPath(mapFile) {}

void DungeonScene::OnEnter() {
    std::cout << "进入 RPG 地牢模式..." << std::endl;

    // 1. 加载地图
    map = new Map();
    map->LoadMap(mapPath);

    // 2. 创建主角 (注意：这里我们不需要重力！)
    player = new RPGPlayer("rpgPlayer.png", Game::renderer, 100, 100, 4,4);

}

void DungeonScene::OnExit() {
    delete player;
    delete map;
}

void DungeonScene::HandleEvents(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_w: player->SetVelY(-4); break; // 上
            case SDLK_s: player->SetVelY(4);  break; // 下
            case SDLK_a: player->SetVelX(-4); break; // 左
            case SDLK_d: player->SetVelX(4);  break; // 右
        }
    }
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
            case SDLK_s: player->SetVelY(0); break;
            case SDLK_a:
            case SDLK_d: player->SetVelX(0); break;
        }
    }
}

void DungeonScene::Update() {
    player->Update();

    // RPG 碰撞检测
    std::vector<SDL_Rect> obstacles = map->getColliders();
    Physics::ResolveRPGCollision(player, obstacles);

    // 简单的摄像机跟随
    Game::camera.x = player->GetBounds().x - 400;
    Game::camera.y = player->GetBounds().y - 300;

    // 边界检查
    if(Game::camera.x < 0) Game::camera.x = 0;
    if(Game::camera.y < 0) Game::camera.y = 0;
}

void DungeonScene::Render() {
    map->DrawMap();
    player->Render();
}

static SceneFactory::Proxy proxy_dungeon("DungeonScene", [](const json& params) {
    std::string mapPath = params.value("map_file", "dungeon.map");
    return new DungeonScene(mapPath);
});