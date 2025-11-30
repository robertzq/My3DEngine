// src/Scenes/DungeonScene.cpp
#include "DungeonScene.h"
#include "Game.h"
#include "Physics.h"
#include "ResourceManager.h"
#include "SceneFactory.h"
#include "RPGPlayer.h"

DungeonScene::DungeonScene(std::string mapFile, bool spawnAtEntrance)
    : mapPath(mapFile), shouldSpawnAtEntrance(spawnAtEntrance) {}

void DungeonScene::OnEnter() {
    std::cout << "进入地图: " << mapPath << std::endl;

    // 1. 加载地图
    map = new Map();
    map->LoadMap(mapPath);

    // 2. 确定主角出生位置
    int startX = 100;
    int startY = 100;

    // 如果指定了出生在入口 (ID: 15)，则扫描地图
    if (shouldSpawnAtEntrance) {
        std::vector<SDL_Rect> entrances = map->GetTiles(15);
        if (!entrances.empty()) {
            startX = entrances[0].x;
            startY = entrances[0].y;
            // 稍微往上一点，不要卡在墙里
            startY -= 10;
        }
    }

    player = new RPGPlayer("rpgPlayer.png", Game::renderer, startX, startY, 4, 4);

    // 3. 加载触发器
    // ID 7: 门 (Village -> House)
    doorTriggers = map->GetTiles(7);

    // ID 14: 舞台/Boss (House -> Battle)
    bossTriggers = map->GetTiles(14);
}

void DungeonScene::OnExit() {
    delete player;
    delete map;
}

void DungeonScene::HandleEvents(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_w: player->SetVelY(-4); break;
            case SDLK_s: player->SetVelY(4);  break;
            case SDLK_a: player->SetVelX(-4); break;
            case SDLK_d: player->SetVelX(4);  break;
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

    // 1. 墙壁碰撞
    std::vector<SDL_Rect> obstacles = map->getColliders();
    Physics::ResolveRPGCollision(player, obstacles);

    // 2. 摄像机跟随
    Game::camera.x = player->GetBounds().x - 400;
    Game::camera.y = player->GetBounds().y - 300;
    if(Game::camera.x < 0) Game::camera.x = 0;
    if(Game::camera.y < 0) Game::camera.y = 0;

    // 3. 检测进门 (Village -> House)
    for (const auto& door : doorTriggers) {
        if (Physics::CheckCollision(player->GetBounds(), door)) {
            std::cout << "进入房子..." << std::endl;
            // 切换到 house.map，并要求出生在入口位置 (true)
            Game::instance()->ChangeScene(new DungeonScene("assets/house.map", true));
            return;
        }
    }

    // 4. 检测 BOSS (House -> Battle)
    for (const auto& bossZone : bossTriggers) {
        if (Physics::CheckCollision(player->GetBounds(), bossZone)) {
            std::cout << "遭遇 BOSS！进入战斗！" << std::endl;
            json params;
            // 可以传参数告诉 BattleScene 是哪个 BOSS
            params["enemy_id"] = "boss_01";
            Scene* battle = SceneFactory::Create("BattleScene", params);
            Game::instance()->ChangeScene(battle);
            return;
        }
    }
}

void DungeonScene::Render() {
    map->DrawMap();
    player->Render();

    // 调试：画出触发器位置
    /*
    SDL_SetRenderDrawColor(Game::renderer, 255, 255, 0, 255); // 黄色门
    for(auto t : doorTriggers) {
        SDL_Rect r = t; r.x -= Game::camera.x; r.y -= Game::camera.y;
        SDL_RenderDrawRect(Game::renderer, &r);
    }
    SDL_SetRenderDrawColor(Game::renderer, 255, 0, 0, 255); // 红色BOSS
    for(auto t : bossTriggers) {
        SDL_Rect r = t; r.x -= Game::camera.x; r.y -= Game::camera.y;
        SDL_RenderDrawRect(Game::renderer, &r);
    }
    SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 255);
    */
}

// 注册工厂
static SceneFactory::Proxy proxy_dungeon("DungeonScene", [](const json& params) {
    std::string mapPath = params.value("map_file", "village.map");
    // 默认不强制出生在入口，除非参数指定
    bool spawnAtEnt = params.value("spawn_at_entrance", false);
    return new DungeonScene(mapPath, spawnAtEnt);
});