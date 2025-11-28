// src/Scenes/DungeonScene.cpp
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

    // 2. 创建主角
    player = new RPGPlayer("rpgPlayer.png", Game::renderer, 100, 100, 4, 4);

    // 3. 【新增】加载所有的 '5' 号方块作为触发器
    triggers = map->GetTiles(5);
    if (!triggers.empty()) {
        std::cout << "加载了 " << triggers.size() << " 个 BOSS 触发点" << std::endl;
    }
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

    // RPG 碰撞检测 (墙壁)
    std::vector<SDL_Rect> obstacles = map->getColliders();
    Physics::ResolveRPGCollision(player, obstacles);

    // 摄像机跟随
    Game::camera.x = player->GetBounds().x - 400;
    Game::camera.y = player->GetBounds().y - 300;

    // 【新增】检测是否踩到触发器 (类型 5)
    for (const auto& trig : triggers) {
        if (Physics::CheckCollision(player->GetBounds(), trig)) {
            std::cout << "踩到触发器！进入 PK！" << std::endl;

            // 切换到 BattleScene
            json params;
            Scene* battle = SceneFactory::Create("BattleScene", params);
            Game::instance()->ChangeScene(battle);
            return; // 切换后直接返回，防止后续逻辑报错
        }
    }

    // 边界检查
    if(Game::camera.x < 0) Game::camera.x = 0;
    if(Game::camera.y < 0) Game::camera.y = 0;
}

void DungeonScene::Render() {
    map->DrawMap();
    player->Render();

    // --- 绘制触发器 (调试用) ---
    // 1. 设置画笔为红色
    SDL_SetRenderDrawColor(Game::renderer, 255, 0, 0, 255);

    for(auto t : triggers) {
        // 记得减去摄像机位置，否则红框会飘
        SDL_Rect drawRect = t;
        drawRect.x -= Game::camera.x;
        drawRect.y -= Game::camera.y;

        // 画红框 (空心框用 RenderDrawRect，实心用 RenderFillRect)
        SDL_RenderDrawRect(Game::renderer, &drawRect);
    }

    // 2. 【核心修复】画完之后，必须把颜色改回默认色（比如黑色或白色）！
    // 否则下一帧清屏时，整个背景都会变成红色
    SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 255);
}

static SceneFactory::Proxy proxy_dungeon("DungeonScene", [](const json& params) {
    std::string mapPath = params.value("map_file", "dungeon.map");
    return new DungeonScene(mapPath);
});